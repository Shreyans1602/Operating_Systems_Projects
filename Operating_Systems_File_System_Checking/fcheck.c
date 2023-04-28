//Libraries include
#include <stdio.h>
#include <sys/types.h>
#include <sys/mman.h>
#include <sys/stat.h>
#include <unistd.h>
#include <stdlib.h>
#include <string.h>
#include <fcntl.h>
#include <assert.h>
#include <stdbool.h>

//Other dependencies include
#include "types.h"
#include "fs.h"

//Defines
#define BLOCK_SIZE (BSIZE)
#define ROOT_INODE_NUM (ROOTINO)
#define DIR_ENTRY_PER_BLOCK (BLOCK_SIZE / sizeof(struct dirent))
#define T_DIR   1   //For determining type of inode - Directory
#define T_FILE  2   //For determining type of inode - File
#define T_DEV   3   //For determining type of inode - Device

//Global variables
struct dinode *disk_inodes_arr = NULL;  //Pointer to the on disk inodes
struct superblock *super_block = NULL;  //Pointer to the superblock of the file system
int *referenced_inodes_arr = NULL;      //Array to keep track of which inodes have been referenced

//Helper functions
/**
 * @brief: xint.
 * @details: Used to get block address pointers. It is taken from implementation in xv6 from mkfs.reference_count. It is one of the methods used to read indirect block. It allows us to read the block numbers from indirect block.
 * @param: x - address to block.
 * @return block address pointers.
 */
uint xint(uint x)
{
  uint indirect_block_addr = 0;
  char *a = (char *)&indirect_block_addr;
  a[0] = x;
  a[1] = x >> 8;
  a[2] = x >> 16;
  a[3] = x >> 24;
  return indirect_block_addr;
}

/**
 * @brief: rsect.
 * @details: Used to read sector from the given file. It is taken from implementation in xv6 from mkfs.reference_count. It is one of the methods used to read indirect block. It allows us to read the block numbers from indirect block.
 * @param: image_file_handler - file to read.
 * @param: sec - sector.
 * @param: buf - buffer to read into.
 * @return none.
 */
void rsect(int image_file_handler, uint sec, void *buf)
{
  if (lseek(image_file_handler, sec * 512L, 0) != sec * 512L)
  {
    perror("lseek");
    exit(1);
  }

  if (read(image_file_handler, buf, 512) != 512)
  {
    perror("read");
    exit(1);
  }
}

/**
 * @brief: get_dir_entries.
 * @details: Used to get all the directory entries for the given data block.
 * @param: image_file_handler - image file.
 * @param: mmap_address_space - virtual address space start address.
 * @param: num_inodes - inode numbers.
 * @return none.
 */
void get_dir_entries(uint block_number, char *mmap_address_space, uint *num_inodes)
{
  struct dirent *directory_entry = (struct dirent *)(mmap_address_space + block_number * BSIZE);
  int iterator = 0;

  for (iterator = 0; iterator < DIR_ENTRY_PER_BLOCK; iterator++, directory_entry++)
  {
    //If the entry is parent or current directory, skip the entry.
    if ((strcmp(directory_entry->name, ".") == 0) || (strcmp(directory_entry->name, "..") == 0))
    {
      num_inodes[iterator] = 0;
      continue;
    }

    //Otherwise update the number of inodes present in the given entry.
    num_inodes[iterator] = directory_entry->inum;
  }
}

/**
 * @brief: is_inode_in_dir.
 * @details: Used to check if the given inode is referred in a directory or not.
 * @param: image_file_handler - image file.
 * @param: mmap_address_space - virtual address space start address.
 * @return none.
 */
void is_inode_in_dir(int image_file_handler, char *mmap_address_space)
{
  int outer_iterator = 0;
  int inner_iterator = 0;

  //Iterate over all the inodes in the super block.
  for (outer_iterator = 0; outer_iterator < super_block->ninodes; outer_iterator++)
  {
    //If inode represents a directory.
    if (disk_inodes_arr[outer_iterator].type == T_DIR)
    {
      //Get number of entries
      int num_dir_entries = disk_inodes_arr[outer_iterator].size / sizeof(struct dirent);

      //Iterate through all the direct data blocks used by the directory.
      for (inner_iterator = 0; inner_iterator < NDIRECT; inner_iterator++)
      {
        //If the block is not in use.
        if ((disk_inodes_arr[outer_iterator].addrs[inner_iterator] == 0) || (num_dir_entries <= 0))
        {
          continue;
        }

        //Access the directory blocks and get the number of inodes.
        uint *num_inodes = (uint *)calloc(DIR_ENTRY_PER_BLOCK, sizeof(uint));

        if (num_inodes == NULL)
        {
          perror("Memory allocation failed");
          exit(1);
        }

        //Update the number of inodes of the directory.
        get_dir_entries(disk_inodes_arr[outer_iterator].addrs[inner_iterator], mmap_address_space, num_inodes);

        int temp_iterator = 0;

        for (temp_iterator = 0; temp_iterator < DIR_ENTRY_PER_BLOCK; temp_iterator++)
        {
          if (num_inodes[temp_iterator] != 0)
          {
            //Update count for the number of times the inode is referenced.
            referenced_inodes_arr[num_inodes[temp_iterator]]++;
            num_dir_entries--;
          }
        }
        free(num_inodes);
      }

      //Now do the same for indirect block addresses.
      if (disk_inodes_arr[outer_iterator].addrs[NDIRECT] == 0)
      {
        continue;
      }

      uint indirect[NINDIRECT] = {0};

      rsect(image_file_handler, xint(disk_inodes_arr[outer_iterator].addrs[NDIRECT]), (char *)indirect);

      //Iterate through all the indirect data blocks used by the directory.
      for (inner_iterator = 0; inner_iterator < NINDIRECT; inner_iterator++)
      {
        if ((indirect[inner_iterator] == 0) || (num_dir_entries <= 0))
        {
          continue;
        }

        //Access the directory blocks and get the number of inodes.
        uint *num_inodes = (uint *)calloc(DIR_ENTRY_PER_BLOCK, sizeof(uint));

        if (num_inodes == NULL)
        {
          perror("Memory allocation failed");
          exit(1);
        }

        //Update the number of inodes of the directory.
        get_dir_entries(indirect[inner_iterator], mmap_address_space, num_inodes);

        int temp_iterator = 0;

        for (temp_iterator = 0; temp_iterator < DIR_ENTRY_PER_BLOCK; temp_iterator++)
        {
          if (num_inodes[temp_iterator])
          {
            //Update count for the number of times the inode is referenced.
            referenced_inodes_arr[num_inodes[temp_iterator]]++;
            num_dir_entries--;
          }
        }

        free(num_inodes);
      }
    }
  }
}

/**
 * @brief: check_rule_1.
 * @details: Checks for rule-1 violations.
 * @return none.
 */
void check_rule_1(struct dinode *disk_inodes_arr, int iterator)
{
  //Check for bad inode. If the type is not valid, it is not present in bitmap. Also, if size is less then 0 then the inode is bad. Rule-1: Bad inode.
  if ((disk_inodes_arr[iterator].size < 0) || (disk_inodes_arr[iterator].type < T_DIR) || (disk_inodes_arr[iterator].type > T_DEV))
  {
    fprintf(stderr, "ERROR: bad inode.\n");
    exit(1);
  }
}

/**
 * @brief: check_rule_2_direct.
 * @details: Checks for rule-2 violations.
 * @return none.
 */
void check_rule_2_direct(struct dinode *disk_inodes_arr, int outer_iterator, int inner_iterator)
{
  //Is data block pointing to a valid address? Throw error if not. Rule-2: bad direct address.
  if ((disk_inodes_arr[outer_iterator].addrs[inner_iterator] < 0) || (disk_inodes_arr[outer_iterator].addrs[inner_iterator] > super_block->nblocks))
  {
    fprintf(stderr, "ERROR: bad direct address in inode.\n");
    exit(1);
  }
}

/**
 * @brief: check_rule_2_indirect.
 * @details: Checks for rule-2 violations.
 * @return none.
 */
void check_rule_2_indirect(int indirect_block_num)
{
  //Check if a valid data block is pointed by the indirect block. Else throw error. Rule-2: bad indirect address.
  if ((indirect_block_num < 0) || (indirect_block_num > super_block->nblocks))
  {
    fprintf(stderr, "ERROR: bad indirect address in inode.\n");
    exit(1);
  }
}

/**
 * @brief: check_rule_3_for_size.
 * @details: Checks for rule-3 violations.
 * @return none.
 */
void check_rule_3_for_size(int size)
{
  //Check if root directory exists. Rule-3: root directory should exist.
  if (size <= 0)
  {
    fprintf(stderr, "ERROR: root directory does not exist.\n");
    exit(1);
  }
}

/**
 * @brief: check_rule_3_for_root_inode.
 * @details: Checks for rule-3 violations.
 * @return none.
 */
void check_rule_3_for_root_inode(int inode_num)
{
  //Check for validity of root and and present directory. Rule-3: inode number should be 1.
  if (inode_num != ROOTINO)
  {
    fprintf(stderr, "ERROR: root directory does not exist.\n");
    exit(1);
  }
}

/**
 * @brief: check_rule_4_for_present_dir_link.
 * @details: Checks for rule-4 violations.
 * @return incremented reference count if check passed.
 */
int check_rule_4_for_present_dir_link(struct dirent *directory_entry, int outer_iterator, int reference_count)
{
  if (directory_entry->inum == outer_iterator)
  {
    //Increase reference count.
    reference_count++;
  }

  //Rule-4: directory . does not point to itself.
  else
  {
    fprintf(stderr, "ERROR: directory not properly formatted.\n");
    exit(1);
  }

  return reference_count;
}

/**
 * @brief: check_rule_4_for_dir_type_and_format.
 * @details: Checks for rule-4 violations.
 * @return none.
 */
void check_rule_4_for_dir_type_and_format(short type, int reference_count)
{
  //If the inode is not a directory or if the inode is missing either of the . or .. directories, then formatting is not proper. Rule-4: formatting is not proper.
  if ((type == T_DIR) && (reference_count != 2))
  {
    fprintf(stderr, "ERROR: directory not properly formatted.\n");
    exit(1);
  }
}

/**
 * @brief: check_rule_5.
 * @details: Checks for rule-5 violations.
 * @return none.
 */
void check_rule_5(char* block_usage_bitmap, uint block_num)
{
  //Check if address is valid but the data block is not in use. Rule-5: data block address is used but data block marked free in bitmap.
  if (((block_usage_bitmap[block_num / 8]) & (0x1 << (block_num % 8))) == 0)
  {
    fprintf(stderr, "ERROR: address used by inode but marked free in bitmap.\n");
    exit(1);
  }
}

/**
 * @brief: check_rule_6.
 * @details: Checks for rule-6 violations.
 * @return none.
 */
void check_rule_6(char *mmap_address_space, uint first_block, uint* used_blocks_arr)
{
  int interator = 0;

  //If the block is marked in use. Check if the address is actually being used or not. Else throw error. Rule-6: Marked in use but not in use.
  for (interator = 0; interator < first_block + super_block->nblocks; interator++)
  {
    char *block_usage_bitmap = mmap_address_space + (BBLOCK(interator, super_block->ninodes)) * BSIZE;

    if (((block_usage_bitmap[interator / 8]) & (0x1 << (interator % 8))))
    {
      if (used_blocks_arr[interator] == 0)
      {
        fprintf(stderr, "ERROR: bitmap marks block in use but it is not in use.\n");
        exit(1);
      }
    }
  }
}

/**
 * @brief: check_rule_7.
 * @details: Checks for rule-7 violations.
 * @return none.
 */
void check_rule_7(uint is_used)
{
  if (is_used == 1)
  {
    fprintf(stderr, "ERROR: direct address used more than once.\n");
    exit(1);
  }
}

/**
 * @brief: check_rule_8.
 * @details: Checks for rule-8 violations.
 * @return none.
 */
void check_rule_8(uint is_used)
{
  //Give error if block address is already in use. Rule-8: block address already in used
  if (is_used)
  {
    fprintf(stderr, "ERROR: indirect address used more than once.\n");
    exit(1);
  }
}

/**
 * @brief: check_rule_9.
 * @details: Checks for rule-9 violations.
 * @return none.
 */
void check_rule_9(int is_used)
{
  if (is_used == 0)
  {
    fprintf(stderr, "ERROR: inode marked use but not found in a directory.\n");
    free(referenced_inodes_arr);
    exit(1);
  }
}

/**
 * @brief: check_rule_10.
 * @details: Checks for rule-10 violations.
 * @return none.
 */
void check_rule_10(int is_used)
{
  //If inode is not being used, it should not be referred. Rule-10: referred an invalid type of inode i.e inode is free.
  if (is_used != 0)
  {
    fprintf(stderr, "ERROR: inode referred to in directory but marked free.\n");
    free(referenced_inodes_arr);
    exit(1);
  }
}

/**
 * @brief: check_rule_11.
 * @details: Checks for rule-11 violations.
 * @return none.
 */
void check_rule_11(short type, int ref_inode_num, short nliks)
{
  //If the inode is a file. It should be referred for a total number of times same as its number of links. Else throw error. Rule-11: Referrence count is bad.
  if ((type == T_FILE) && (ref_inode_num != nliks))
  {
    fprintf(stderr, "ERROR: bad reference count for file.\n");
    free(referenced_inodes_arr);
    exit(1);
  }
}

/**
 * @brief: check_rule_12.
 * @details: Checks for rule-12 violations.
 * @return none.
 */
void check_rule_12(short type, int ref_inode_num)
{
  //Error when inode is a directory and referred more than once. Rule-12: directory referred more than once.
  if ((type == T_DIR) && (ref_inode_num > 1))
  {
    fprintf(stderr, "ERROR: directory appears more than once in file system.\n");
    free(referenced_inodes_arr);
    exit(1);
  }
}

/**
 * @brief: main.
 * @details: Main function.
 * @return int.
 */
int main(int argc, char *argv[])
{
  //Local variables initialized.
  int image_file_handler = 0;
  char *mmap_address_space = NULL;
  struct dirent *directory_entry = NULL;
  struct stat file_statistics;

  //Input arguments validation.
  if (argc != 2)
  {
    fprintf(stderr, "Usage: fcheck <file_system_image>\n"); //Validation of page 3.
    exit(1);
  }

  image_file_handler = open(argv[1], O_RDONLY);

  //File operation failed - file does not exist.
  if (image_file_handler < 0)
  {
    fprintf(stderr, "image not found.\n"); //Validation of page 3.
    exit(1);
  }

  //Gets the size of the file that the image_file_handler is pointing to as instructed in the problem statement.
  fstat(image_file_handler, &file_statistics);

  //Size not hard-coded as per instructions given.
  mmap_address_space = mmap(NULL, file_statistics.st_size, PROT_READ, MAP_PRIVATE, image_file_handler, 0);

  if (mmap_address_space == MAP_FAILED)
  {
    perror("mmap failed");
    exit(1);
  }

  //Read superblock
  super_block = (struct superblock *)(mmap_address_space + 1 * BLOCK_SIZE);

  //Read disk inodes
  disk_inodes_arr = (struct dinode *)(mmap_address_space + IBLOCK((uint)0) * BLOCK_SIZE);

  int num_inodes = super_block->ninodes;
  int outer_iterator, inner_iterator;

  //Check if root directory exists. Rule-3: root directory should exist.
  check_rule_3_for_size(disk_inodes_arr[ROOTINO].size);

  directory_entry = (struct dirent *)(mmap_address_space + (disk_inodes_arr[ROOTINO].addrs[0]) * BSIZE);
  int size = disk_inodes_arr[ROOTINO].size / sizeof(struct dirent); 
  int reference_count = 0;

  for (outer_iterator = 0; outer_iterator < size; outer_iterator++)
  {
    if ((reference_count < 2) && ((strcmp(directory_entry->name, ".") == 0) || (strcmp(directory_entry->name, "..") == 0)))
    {
      //Check for validity of root and and present directory. Rule-3: inode number should be 1.
      check_rule_3_for_root_inode(directory_entry->inum);
      
      //Check is passed so increase reference count.
      reference_count++;
    }
    directory_entry++;
  }

  //Used to maintain the list of blocks in use.
  uint used_blocks_arr[super_block->size];

  //Block number of the first data block.
  uint first_block = BBLOCK(super_block->size, super_block->ninodes) + 1;

  //Initially, no blocks are in use.
  for (outer_iterator = 0; outer_iterator < super_block->size; outer_iterator++)
  {
    used_blocks_arr[outer_iterator] = 0;
  }

  //Mark all blocks before the first data block in use since they are supernode, inodes and bitmap.
  for (outer_iterator = 0; outer_iterator < first_block; outer_iterator++)
  {
    used_blocks_arr[outer_iterator] = 1;
  }

  //Bad inode check by iterating through individual inodes.
  for (outer_iterator = 0; outer_iterator < num_inodes; outer_iterator++)
  {
    //The inode is not in use.
    if (disk_inodes_arr[outer_iterator].size == 0)
    {
      continue;
    }

    //Check for bad inode. If the type is not valid, it is not present in bitmap. Also, if size is less then 0 then the inode is bad. Rule-1: Bad inode.
    check_rule_1(disk_inodes_arr,outer_iterator);

    reference_count = 0;

    //For each direct entry address.
    for (inner_iterator = 0; inner_iterator < NDIRECT; inner_iterator++)
    {
      //Check if the data block is not in use.
      if (disk_inodes_arr[outer_iterator].addrs[inner_iterator] == 0)
      {
        continue;
      }

      //Is data block pointing to a valid address? Throw error if not. Rule-2: bad direct address.
      check_rule_2_direct(disk_inodes_arr, outer_iterator, inner_iterator);

      //Get the block number of the data block in use.
      uint block_num = disk_inodes_arr[outer_iterator].addrs[inner_iterator];

      char *block_usage_bitmap = mmap_address_space + (BBLOCK(block_num, super_block->ninodes)) * BSIZE;

      //Check if address is valid but the data block is not in use. Rule-5: data block address is used but data block marked free in bitmap.
      check_rule_5(block_usage_bitmap, block_num);

      //Is current data block used already? If yes then throw error. Rule-7: Using direct address more than once.
      check_rule_7(used_blocks_arr[block_num]);

      //Now mark this data block as in use.
      used_blocks_arr[block_num] = 1;

      //See if each directory has current and parent directories.
      if (disk_inodes_arr[outer_iterator].type == T_DIR)
      {
        directory_entry = (struct dirent *)(mmap_address_space + block_num * BSIZE);
        int size = disk_inodes_arr[outer_iterator].size / sizeof(struct dirent);
        int temp_iterator = 0;

        for (temp_iterator = 0; temp_iterator < size; temp_iterator++, directory_entry++)
        {
          if (reference_count < 2)
          {
            if (strcmp(directory_entry->name, ".") == 0)
            {
              reference_count = check_rule_4_for_present_dir_link(directory_entry, outer_iterator, reference_count);
            }

            else if (strcmp(directory_entry->name, "..") == 0)
            {
              reference_count++;
            }
          }
        }
      }
    }

    //If the inode is not a directory or if the inode is missing either of the . or .. directories, then formatting is not proper. Rule-4: formatting is not proper.
    check_rule_4_for_dir_type_and_format(disk_inodes_arr[outer_iterator].type, reference_count);

    //Skip direct blocks now since checking is done.
    if (disk_inodes_arr[outer_iterator].addrs[NDIRECT] == 0)
    {
      continue;
    }

    uint temp_inode_addr = disk_inodes_arr[outer_iterator].addrs[NDIRECT];

    //The indirect block address is being used so mark it in list of used blocks.
    if ((temp_inode_addr > 0) && (temp_inode_addr < super_block->size))
    {
      used_blocks_arr[temp_inode_addr] = 1;
    }

    uint indirect[NINDIRECT] = {0};

    //Get indirect block address in similar fashion to xv6.
    uint indirect_block_addr = xint(temp_inode_addr);
    rsect(image_file_handler, indirect_block_addr, (char *)indirect);

    //Iterate through all the indirect blocks.
    for (inner_iterator = 0; inner_iterator < NINDIRECT; inner_iterator++)
    {
      //Get indirect block number.
      uint indirect_block_num = indirect[inner_iterator];

      //If the address of this block is not being used, skip it.
      if (indirect_block_num == 0)
      {
        continue;
      }

      //Check if a valid data block is pointed by the indirect block. Else throw error. Rule-2: bad indirect address.
      check_rule_2_indirect(indirect_block_num);

      //Check if the block is marked in use in bitmap and is actually being used. Get the block usage bitmap.
      char *block_usage_bitmap = mmap_address_space + (BBLOCK(indirect_block_num, super_block->ninodes)) * BSIZE;

      //Rule-5: block marked free in bitmap while the address is being used. Throw error.
      check_rule_5(block_usage_bitmap, indirect_block_num);

      //Give error if block address is already in use. Rule-8: block address already in used
      check_rule_8(used_blocks_arr[indirect_block_num]);

      //Now mark the data block as in use.
      used_blocks_arr[indirect_block_num] = 1;
    }
  }

  //If the block is marked in use. Check if the address is actually being used or not. Else throw error. Rule-6: Marked in use but not in use.
  check_rule_6(mmap_address_space, first_block, used_blocks_arr);

  referenced_inodes_arr = (int *)calloc(super_block->ninodes, sizeof(int));

  if (referenced_inodes_arr == NULL)
  {
    perror("Memory allocation failed");
    exit(1);
  }

  //Get the list of referenced inodes in the directory.
  is_inode_in_dir(image_file_handler, mmap_address_space);

  //Check all inodes after the first two (. and .. are not to be counted so we start from 2)
  for (outer_iterator = 2; outer_iterator < super_block->ninodes; outer_iterator++)
  {
    if (disk_inodes_arr[outer_iterator].type == 0)
    {
      //If inode is not being used, it should not be referred. Rule-10: referred an invalid type of inode i.e inode is free.
      check_rule_10(referenced_inodes_arr[outer_iterator]);

      //Check passed. Go for nexxt inode.
      continue;
    }

    //If inode is being used, it should be referred atleast once in a directory. Else throw error. Rule-9: Inode not found in directory but marked in use.
    check_rule_9(referenced_inodes_arr[outer_iterator]);

    //If the inode is a file. It should be referred for a total number of times same as its number of links. Else throw error. Rule-11: Referrence count is bad.
    check_rule_11(disk_inodes_arr[outer_iterator].type, referenced_inodes_arr[outer_iterator], disk_inodes_arr[outer_iterator].nlink);

    //Error when inode is a directory and referred more than once. Rule-12: directory referred more than once.
    check_rule_12(disk_inodes_arr[outer_iterator].type, referenced_inodes_arr[outer_iterator]);
  }

  exit(0);
}