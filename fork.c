#include <stdio.h>
#include <stdlib.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>
#include <string.h>
#include <wait.h>

int main(int argc,char *argv[])
{
    pid_t pid;
    int i,status;
    for(i=0;i<4;i++)
    {
        pid=fork();
        if(pid<0)
        {
            perror("fork error");
            exit(EXIT_FAILURE);
        }

        if(pid==0)
        {
            if(i==3)
            {
                printf("child process %d is sleeping %d seconds\n",getpid(),1);
                sleep(1);
                int *p=NULL;
                *p=1;
                exit(40);
            }

            else{
            printf("child process %d is sleeping %d seconds\n",getpid(),i+1);
            sleep(i+1);
            printf("child process %d is exiting\n",getpid());
            fflush(stdout);
            _exit((i+1)*10);
            }
        }
    }

    for(i=0;i<4;i++)
    {
        pid=waitpid(-1,&status,0);
        if(WIFEXITED(status)){
            printf("parent process pid %d has reaped child process %d,exitstatus<%d>\n",getpid(),pid,WEXITSTATUS(status));
        }
        else{
            printf("子进程%d异常退出,异常编号%d",pid,WTERMSIG(status));
        }
    }


}