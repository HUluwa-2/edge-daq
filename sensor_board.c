#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <fcntl.h>
#include <string.h>
#include <sys/stat.h>
#include <sys/ioctl.h>

#define AP3216C_GET_ALS     _IOR('D',1,unsigned short)
#define AP3216C_GET_PS      _IOR('D',2,unsigned short)

int main(int argc,char *argv[])
{
    if(argc!=3)
    {
        fprintf(stderr,"usage:%s<sensor_name><num>\n",argv[0]);
        exit(EXIT_FAILURE);
    }

    char* ch=argv[1];
    if(strcmp(ch,"temp")&&strcmp(ch,"prox")&&strcmp(ch,"light"))
    {
        fprintf(stderr,"error:sensor_name must be temp,prox or light\n");
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
    int fq;
    signed int databuf[7];
    unsigned short als,ps;
    if(!strcmp(ch,"temp")){
        fq=open("/dev/icm20608",O_RDWR);
        if(fq<0)
        {
            perror("open error");
            exit(EXIT_FAILURE);
        }
        for(int i=0;i<num_msgs;i++)
        {   
            if(0>read(fq,databuf,sizeof(databuf)))
        {
            perror("read error");
        }
            float temp=((float)databuf[6]-25)/326.8 + 25;

            snprintf(buffer,sizeof(buffer),"%s seq=%d value=%.1f\n",argv[1],i+1,temp);
            write(fd,buffer,strlen(buffer));
            sleep(1);
        }
    }
    else if(!strcmp(ch,"prox")){
        fq=open("/dev/ap3216c",O_RDWR);
        if(fq<0)
        {
            perror("open error");
            exit(EXIT_FAILURE);
        }
        for(int i=0;i<num_msgs;i++)
        {   
            ioctl(fq,AP3216C_GET_PS,&ps);

            snprintf(buffer,sizeof(buffer),"%s seq=%d value=%u\n",argv[1],i+1,ps);
            write(fd,buffer,strlen(buffer));
            sleep(1);
        }
    }
    else{
        fq=open("/dev/ap3216c",O_RDWR);
        if(fq<0)
        {
            perror("open error");
            exit(EXIT_FAILURE);
        }
        for(int i=0;i<num_msgs;i++)
        {   
            ioctl(fq,AP3216C_GET_ALS,&als);

            snprintf(buffer,sizeof(buffer),"%s seq=%d value=%u\n",argv[1],i+1,als);
            write(fd,buffer,strlen(buffer));
            sleep(1);
        }
    }
    printf("[Writer] 数据写入完成,共发送%d条\n",num_msgs);
    close(fd);
    close(fq);
    return 0;
}