# Operating_Systems_Introduction_to_Kernel_Programming

# Add a system call to xv6
In this exercise, you will write a new system call getreadcount() for xv6 OS. The return type and parameters are:

int getreadcount(void)

The call returns the total number of times the read() system call has been invoked since the time the kernel was booted. Note that the kernel does not keep a counter. You must add it.

# Background
The required background to implement this exercise will be discussed in the class. You can also watch the discussion video: https://www.youtube.com/watch?v=vR6z2QGcoo8, by Prof. Remzi to understand more about xv6 and systems calls. You should also read (several times) this exercise’s description. Chapter 3 of the xv6 book contains details on traps and system calls. However, you may not need to understand most of the low-level stuff (on interrupt tables and trap frames) to complete this exercise.

# xv6 Source code
The xv6 source code for this exercise is xv6.tar.gz. Copy this file to your local working directory and extract the source code tree using the command:

tar -zxvf xv6.tar.gz

# Building and Running xv6
In the xv6 directory, you will find: 

FILES   include   kernel  Makefile  Makefile~   README  tools   user  version 

Look at FILES using vim for a description of each directory.

## Compile the xv6 kernel:
In the xv6 directory, type make. When the compilation is complete, you should now find two additional files in the xv6 directory: 

fs.img  xv6.img

## Run xv6 on QEMU:
Xv6 is developed for x86 architecture. To run it, you need an emulator. You will use QEMU. The command to run xv6 on QEMU is: 

$ make qemu 

You may see a couple of warnings, then you should see the following output lines as xv6 boots up: 

xv6... 
lapicinit: 1 0xfee00000 
cpu1: starting 
cpu0: starting 
init: starting sh 
$

## Take a tour of xv6:
You are now in a xv6 shell. Type ls to see the list of programs that you can run. You should see a few familiar commands such as 'cat' and 'mkdir'. Try: 

$ cat README

Now try running the rest of the programs to get a feel for what they do. If a program does not exit on its own, you can kill it by typing: 

control+d

## Exit xv6 and QEMU:
You can exit xv6 by typing: 

control+a, then release both keys and type x

## Run xv6 again, this time with GDB remote debugging enabled:
The GNU Debugger (GDB) will be one of your best friends this semester. xv6 allows you to "look inside" while it is running using GDB. This requires two terminals: one to run xv6 and another to run GDB with remote debugging.

First, run xv6 with remote debugging enabled: 

$ make qemu-gdb 

You will notice that this time xv6 hangs during the boot-up process. It is waiting for you to make a remote connection using GDB. In another terminal window, in the xv6 directory, type: 

$ gdb kernel/kernel 

The first time you will probably get a warning that says: 

...auto-loading has been declined by your "auto-load safe-path"...

Fortunately, it also tells you how to fix the problem: 

To enable execution of this file add 
    add-auto-load-safe-path /<path-to-xv6>/xv6/.gdbinit
line to your configuration file "/home/<username>/.gdbinit". 
To completely disable this security protection add 
    set auto-load safe-path /
line to your configuration file "/home/<username>/.gdbinit".

Go ahead and quit out of GDB so that you can follow the message instructions. You'll probably have to create a .gdbinit file in your home directory (there is already a separate .gdbinit in your xv6 directory, which you should NOT touch--leave it be). You should create (or, if it already exists, modify) ~/.gdbinit as suggested in the message. We recommend using the add-auto-load-safe-path method, which we have highlighted above.
  
Once you have fixed your .gdbinit file, you can run GDB again: 
  
$ gdb kernel/kernel 
  
This time, when GDB starts up, you should see the following lines in the output: 
  
  Connecting to QEMU 
  (gdb) target remote localhost:25273 (<--port number may differ) 
                                               
Now lets insert a breakpoint in the exec function. This kernel-level function is invoked whenever a new process is executed. In GDB, you can set the breakpoint by typing: 
                                               
(gdb) b exec 
                                               
We are now ready to let xv6 finish booting. In GDB, let xv6 continue by typing: 
                                               
(gdb) c 
                                               
In your xv6 terminal, you should see the system begin to boot: 
                                               
  xv6... 
  lapicinit: 1 0xfee00000
  cpu1: starting
  cpu0: starting 
                                               
Then it hits the exec function for the first time, triggering the breakpoint. In GDB, you should see: 
                                               
Breakpoint 1, exec (path=0x1c "/init", argv=0xff0e98) at kernel/exec.c:11 11 { 
                                               
Notice the part that says: path=0x1c "/init" 
                                               
This means that exec is about to execute the init process. We have just discovered the first process that xv6 executes during boot-up!
                                               
Keep typing 'c' or "continue" in GBD until xv6 reaches the shell prompt, noting the process(es) executed along the way. Once at the shell prompt, you can type an xv6 command to run a program, which should trigger your breakpoint again. You can then type 'c' in GDB to allow the program to run to completion. NOTE: Some programs may behave differently when remote debugging is in use. You can now repeat your previous tour of xv6, but this time you can set breakpoints and use other GDB features to do some more snooping around.
                                               
## Exit xv6, QEMU, and GDB:
                                               
You can exit xv6 and QEMU as before by typing control+a then x. You can exit GDB by typing q and then enter.
                                               
# Hints
You need to modify several files to implement this exercise. Carefully look at these files: syscall.h, syscall.c, sysfile.c, user.h, and usys.S. Study how other system calls such getpid(), uptime() are implemented. You have to do something similar for this exercise.
