#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <fcntl.h>
#include <sys/stat.h>

int main()
{
    const char *fifo_path="/tmp/my_fifo";
    char buffer[128];

    mkfifo(fifo_path,0666);

    printf("[Reader] 准备从管道读取数据...\n");
    int fd=open(fifo_path,O_RDONLY);
    if(fd<0)
    {
        perror("open error");
        exit(EXIT_FAILURE);
    }
    printf("[Reader] 从管道读取数据...\n");
    ssize_t bytes_read=read(fd,buffer,sizeof(buffer)-1);
    if(bytes_read>=0)
    {
        buffer[bytes_read]='\0';
        printf("[Reader] 从管道读取到数据: %s\n",buffer);
    }
    else
    {
        perror("read error");
    }

    close(fd);
    return 0;
}