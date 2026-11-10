#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <fcntl.h>
#include <string.h>
#include <sys/stat.h>
#include <time.h>

int main(int argc,char *argv[])
{
    if(argc!=3)
    {
        fprintf(stderr,"usage:%s<sensor_name><num>\n",argv[0]);
        exit(EXIT_FAILURE);
    }

    char* ch=argv[1];
    if(strcmp(ch,"temp")&&strcmp(ch,"humi")&&strcmp(ch,"light"))
    {
        fprintf(stderr,"error:sensor_name must be temp,humi or light\n");
        exit(EXIT_FAILURE);
    }
    int num_msgs=atoi(argv[2]);
    if(num_msgs<=0)
    {
        fprintf(stderr,"error:<num>must be a positive integer\n");
        exit(EXIT_FAILURE);
    }

    const char *fifo_path="/tmp/edgedaq.fifo";
    mkfifo(fifo_path,0666);

    printf("[Writer] 准备向管道写入数据...\n");
    int fd=open(fifo_path,O_WRONLY);
    if(fd<0)
    {
        perror("open error");
        exit(EXIT_FAILURE);
    }

    printf("[Writer] 向管道写入数据...\n");
    char buffer[256];

    srand(time(NULL)^getpid());
    if(!strcmp(ch,"temp")){
        for(int i=0;i<num_msgs;i++)
        {   
            float value=25.0+(rand()%30)/10.0;

            snprintf(buffer,sizeof(buffer),"%s seq=%d value=%.1f\n",argv[1],i+1,value);
            write(fd,buffer,strlen(buffer));
            sleep(1);
        }
    }
    else if(!strcmp(ch,"humi")){
        for(int i=0;i<num_msgs;i++)
        {   
            float value=40.0+(rand()%200)/10.0;

            snprintf(buffer,sizeof(buffer),"%s seq=%d value=%.1f\n",argv[1],i+1,value);
            write(fd,buffer,strlen(buffer));
            sleep(1);
        }
    }
    else{
        for(int i=0;i<num_msgs;i++)
        {   
            int value=100+(rand()%800);

            snprintf(buffer,sizeof(buffer),"%s seq=%d value=%d\n",argv[1],i+1,value);
            write(fd,buffer,strlen(buffer));
            sleep(1);
        }
    }
    printf("[Writer] 数据写入完成,共发送%d条\n",num_msgs);
    close(fd);
    return 0;
}