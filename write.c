#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <fcntl.h>
#include <string.h>
#include <sys/stat.h>

int main()
{
    const char *fifo_path="/tmp/my_fifo";
    const char *message="Hello from writer process!";
    mkfifo(fifo_path,0666);

    printf("[Writer] 准备向管道写入数据...\n");
    int fd=open(fifo_path,O_WRONLY);
    if(fd<0)
    {
        perror("open error");
        exit(EXIT_FAILURE);
    }

    printf("[Writer] 向管道写入数据...\n");
    write(fd,message,strlen(message));
    printf("[Writer] 数据写入完成\n");
    close(fd);
    return 0;
}