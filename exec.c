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
    for(i=0;i<3;i++)
    {
        pid=fork();
        if(pid<0)
        {
            perror("fork error");
            exit(EXIT_FAILURE);
        }

        if(pid==0)
        {
            if(i==0)
            {
                printf("son possess:%d\n",getpid());
                if(0>execlp("ls","ls","-a","-l",NULL))
                {
                    perror("execlp error");
                    _exit(EXIT_FAILURE);        
                }
            }
            else if(i==1)
            {
                printf("son possess:%d\n",getpid());
                char *date_args[]={"date",NULL};
                if(0>execvp("date",date_args))
                {
                    perror("execvp error");
                    _exit(EXIT_FAILURE);  
                }
            }
            else
            {
                printf("son possess:%d\n",getpid());
                if(0>execl("/bin/ps","ps",NULL))
                {
                    perror("execl error");
                    _exit(EXIT_FAILURE);  
                }
            }
        }
    }

    for(i=0;i<3;i++)
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