#include "types.h"
#include "stat.h"
#include "user.h"
#include "pstat.h"
#include "fcntl.h"

void 
spin()
{
    int i = 0;
    int j = 0;
    int k = 0;
    for(i = 0; i < 50; ++i)
    {
        for(j = 0; j < 400000; ++j)
        {
            k = j % 10;
            k = k + 1;
        }
    }
}

int
main(int argc, char *argv[])
{
    int number_of_tickets[3] = {30,20,10};
    int number_of_ticks[3] = {0,0,0};
    int child_pid[3] = {0,0,0};
    int i = 0;
    int j = 0;
    int pid = 0;
    int iteration_time = 0;

    struct pstat pstat_struct;
    
    child_pid[0] = getpid();

    settickets(number_of_tickets[0]);

    for(i=1;i<3;i++)
    {
        child_pid[i] = fork();
        
        if(child_pid[i] == 0)
        {
            settickets(number_of_tickets[i]);
            for (;;)
            {
                spin();
            }
        }
    }
        
    printf(1,"Process PID:\t\t%d\t%d\t%d\n", child_pid[0], child_pid[1], child_pid[2]);
    printf(1,"Process Tickets:\t%d\t%d\t%d\n", number_of_tickets[0], number_of_tickets[1], number_of_tickets[2]);

    while(iteration_time < 100)
    {
        if(getpinfo(&pstat_struct) != 0)
        {
            printf(1,"check failed: getpinfo\n");
            goto Exit;
        }

        for(i=0;i<3;i++)
        {
            pid = child_pid[i];

            for(j=0; j<NPROC; j++)
            {
                if(pstat_struct.pid[j] == pid)
                {
                    number_of_ticks[i] = pstat_struct.ticks[j];
                }
            }
        }

        printf(1,"Iteration Number: %d\t",iteration_time+1);

        for(i=0; i<3; i++)
        {
            printf(1,"%d\t",number_of_ticks[i]);
        }

        printf(1,"\n");
        
        spin();
        iteration_time++;
    }
        
    Exit:
    for (i = 0; child_pid[i] > 0; i++)
    {
        kill(child_pid[i]);
    }

    while(wait() > -1);

    exit();
}
