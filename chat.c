#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <fcntl.h>
#include <sys/stat.h>
#include <pthread.h>

#define FIFO_A "/tmp/chat_a.fifo"
#define FIFO_B "/tmp/chat_b.fifo"

void *receisive_thread_func(void *arg)
{
    int fd=*(int *)arg;
    char buffer[256];

    while(1)
    {
        ssize_t bytes=read(fd,buffer,sizeof(buffer)-1);
        if(bytes<=0)
        {
            printf("对方已退出聊天\n");
            exit(EXIT_SUCCESS);
        }
        buffer[bytes]='\0';
        printf("对方说: %s\n",buffer);
    }
    return NULL;
}

int main(int argc,char *argv[])
{   
    if (argc != 2) { fprintf(stderr, "Usage: %s [a|b]\n", argv[0]); 
        exit(EXIT_FAILURE); }
    char role = argv[1][0];                       
    if (role != 'a' && role != 'b') 
    {   
        fprintf(stderr, "Usage: %s [a|b]\n", argv[0]); 
        exit(EXIT_FAILURE); 
    }

    int fd_w,fd_r;

    mkfifo(FIFO_A,0666);
    mkfifo(FIFO_B,0666);

    if(role=='a')
    {
        printf("你是A角色,请等待B角色加入聊天...\n");
        fd_w=open(FIFO_A,O_WRONLY);
        if(fd_w<0)
        {
            perror("open error");
            exit(EXIT_FAILURE);
        }
        fd_r=open(FIFO_B,O_RDONLY);
        if(fd_r<0)
        {
            perror("open error");
            exit(EXIT_FAILURE);
        }
        printf("你是A角色,B角色已加入聊天,你可以开始聊天了\n");
    }
    else
    {
        printf("你是B角色,请等待A角色加入聊天...\n");
        fd_r=open(FIFO_A,O_RDONLY);
        if(fd_r<0)
        {
            perror("open error");
            exit(EXIT_FAILURE);
        }
        fd_w=open(FIFO_B,O_WRONLY);
        if(fd_w<0)
        {
            perror("open error");
            exit(EXIT_FAILURE);
        }
        printf("你是B角色,已加入聊天,你可以开始聊天了\n");
    }

    pthread_t receisive_tid;
    if(pthread_create(&receisive_tid,NULL,receisive_thread_func,&fd_r)!=0)
    {
        fprintf(stdout,"pthread_create error\n");
        exit(EXIT_FAILURE);
    }

    char buffer[256];
    while(1)
    {
        if(fgets(buffer,sizeof(buffer),stdin)==NULL)
        {
            break;
        }

        write(fd_w,buffer,strlen(buffer));
    }

    close(fd_w);
    close(fd_r);
    return 0;
}