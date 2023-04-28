# Operating_Systems_Print_Page_Tables_in_xv6

# Print page tables
To help visualize page tables, you need to write a function that prints the contents of the page tables. Specifically, define function: 

ptprint(pde_t *pgdir)

It will take pde_t as argument and print the page table tree as shown below. Insert the following code before return 0 in exec.c, to print the page tables of the first process:

    if(proc->pid == 1)
    { 
      ptprint(proc->pgdir); 
    } 

Now when you start xv6 it should print output like this showing the entries of the page directory and page tables of the first process at the point xv6 has finished exec()ing init.

    pgdir 0xfe1000 
    ..0: pde 0xfe0027 pa 0xfe0000 
    .. ..0: pte 0xfd4007 pa 0xfd4000 
    .. ..1: pte 0xfd3007 pa 0xfd3000 
    ..1: pde 0xfdf007 pa 0xfdf000 
    ..2: pde 0xfde007 pa 0xfde000 
    ..3: pde 0xfdd027 pa 0xfdd000 
    ..1016: pde 0xfdc007 pa 0xfdc000 
    ..1017: pde 0xfdb007 pa 0xfdb000 
    ..1018: pde 0xfda007 pa 0xfda000 
    ..1019: pde 0xfd9027 pa 0xfd9000 
    ..1020: pde 0xfd8007 pa 0xfd8000 
    ..1021: pde 0xfd7007 pa 0xfd7000 
    ..1022: pde 0xfd6007 pa 0xfd6000 
    ..1023: pde 0xfd5007 pa 0xfd5000
    
The first line displays the argument to ptprint. After that there is a line for each PDE/PTE, including PTEs that refer to page-table pages deeper in the tree. Each line is indented by a number of " .." that indicates its depth in the tree. Each PTE line shows the PTE index in its page-table page, the pte bits, and the physical address extracted from the PTE. Print only those PDEs/PTEs that are valid (PTE_P) and with user privilege (PTE_U). In the above example, the page directory page has mappings for entries 0 to 3 and entries 1016 to 1023. The next level down for entry 0 has entries 0, 1, and 2 mapped

# Hints
* You can put ptprint() in vm.c 
* Declare the prototype for ptprint() in defs.h. Declare it in the appropriate location in the file.
* Look at walkpgdir(). Your code will be somewhat similar.
* Understand the macros used in walkpgdir(). They are defined in mmu.h

It will also be insightful to see the page tables for other processes. Remove the if condition, and call ptprint unconditionally in exec.c. See the page table entries for sh. Type ‘ls’ (or any other command you fancy) to see the page table entries of the newly created process.
