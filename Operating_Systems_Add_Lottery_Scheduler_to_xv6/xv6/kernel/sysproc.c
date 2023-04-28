#include "types.h"
#include "x86.h"
#include "defs.h"
#include "param.h"
#include "mmu.h"
#include "proc.h"
#include "sysfunc.h"

/** 
 * [PROJECT-2]: The following code is added by Shreyans (SSP210009) and Karan (KHJ200000)
 * Added pstat.h include file for pstat structure usage
**/
#include "pstat.h"

/* End of code added */

int
sys_fork(void)
{
  return fork();
}

int
sys_exit(void)
{
  exit();
  return 0;  // not reached
}

int
sys_wait(void)
{
  return wait();
}

int
sys_kill(void)
{
  int pid;

  if(argint(0, &pid) < 0)
    return -1;
  return kill(pid);
}

int
sys_getpid(void)
{
  return proc->pid;
}

int
sys_sbrk(void)
{
  int addr;
  int n;

  if(argint(0, &n) < 0)
    return -1;
  addr = proc->sz;
  if(growproc(n) < 0)
    return -1;
  return addr;
}

int
sys_sleep(void)
{
  int n;
  uint ticks0;
  
  if(argint(0, &n) < 0)
    return -1;
  acquire(&tickslock);
  ticks0 = ticks;
  while(ticks - ticks0 < n){
    if(proc->killed){
      release(&tickslock);
      return -1;
    }
    sleep(&ticks, &tickslock);
  }
  release(&tickslock);
  return 0;
}

// return how many clock tick interrupts have occurred
// since boot.
int
sys_uptime(void)
{
  uint xticks;
  
  acquire(&tickslock);
  xticks = ticks;
  release(&tickslock);
  return xticks;
}

/** 
 * [PROJECT-2]: The following code is added by Shreyans (SSP210009) and Karan (KHJ200000)
 * Added two new system calls here. Their definitions are given below
**/

/**
 * @brief sys_settickets - This function is used to set the number of tickets of the calling process. By default, each new 
 * process will have atleast 1 ticket by default. This function will change the number of tickets with the process.
 * @return int - If successfull return 0 else return -1
 */
int 
sys_settickets(void)
{
  int tickets;  //Variable to fetch the number of tickets argument in

  //Fetch the argument and assign it to tickets
  if (argint(0,&tickets) < 0)
  {
    return -1;
  }

  //If trying to assign negative or 0 tickets, give error
  if (tickets <= 0)
  {
    return -1;
  }
  
  //Assign the tickets to the process
  proc->tickets = tickets;

  return 0;
}

/**
 * @brief sys_getpinfo - This function is used to get the info about the process similar to ps function. It gives the data 
 * present in the pstat structure
 * @return int - If successfull return 0 else return -1
 */
int 
sys_getpinfo(void)
{
  struct proc* process;
  struct pstat* proc_struct;

  acquire(&ptable.lock);

  //Bad pointer passed to the function. Give error
  if(argptr(0, (char **)(&proc_struct), sizeof(proc_struct)) < 0) 
  {
    release(&ptable.lock);
    return -1;
  }

  //Bad NULL pointer passed to the function. Give error
  if(NULL == proc_struct)
  {
    release(&ptable.lock);
    return -1;
  }

  //Iterate through the processes in the table and update pstat structure with the details of processes which are not unused
  for(process = ptable.proc; process != &(ptable.proc[NPROC]); process++) 
  {
    int writeIndex = process - ptable.proc;
    if(process->state != UNUSED) 
    {
	      proc_struct->pid[writeIndex] = process->pid;
        proc_struct->ticks[writeIndex] = process->ticks;
        proc_struct->tickets[writeIndex] = process->tickets;
        proc_struct->inuse[writeIndex] = process->inuse;
    }
  }
  release(&ptable.lock);
  return 0;
}
/* End of code added */