#include <stdio.h>
#include <stdlib.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>
#include <string.h>
#include <wait.h>
#include <time.h>

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
            _exit(EXIT_FAILURE);
        }

        if(pid==0)
        {
            if(i==0)
            {
                printf("son possess:%d\n",getpid());
                if(0>execlp("ls","ls","-a","-l",NULL))
                {
                    perror("execlp error");
                    _exit(127);        
                }
            }
            else if(i==1)
            {
                printf("son possess:%d\n",getpid());
                char *data_args[]={"data",NULL};
                if(0>execvp("data",data_args))
                {
                    perror("execvp error");
                    _exit(127);  
                }
            }
            else
            {
                printf("son possess:%d\n",getpid());
                if(0>execl("/bin/ps","ps",NULL))
                {
                    perror("execl error");
                    _exit(127);  
                }
            }
        }
    }

    FILE *file=fopen("run.log","a+");
    if(file==NULL)
    {
        perror("open error");
        exit(EXIT_FAILURE);
    }
    for(i=0;i<3;i++)
    {
        
        time_t rawtime;
        struct tm *timeinfo;
        char buffer[80];
        

        pid=waitpid(-1,&status,0);
        if(WIFEXITED(status)){
            time(&rawtime);
            timeinfo=localtime(&rawtime);
            strftime(buffer,sizeof(buffer),"%Y-%m-%d %H:%M:%S ",timeinfo);
            fprintf(file,"%s",buffer);
            fprintf(file,"parent process pid %d has reaped child process %d,exitstatus<%d>\n",getpid(),pid,WEXITSTATUS(status));
            fflush(file);
        }
        else{
            time(&rawtime);
            timeinfo=localtime(&rawtime);
            strftime(buffer,sizeof(buffer),"%Y-%m-%d %H:%M:%S ",timeinfo);
            fprintf(file,"%s",buffer);
            fprintf(file,"子进程%d异常退出,异常编号%d\n",pid,WTERMSIG(status));
            fflush(file);
        }
    }
    fclose(file);
    exit(0);
}
