#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <fcntl.h>
#include <sys/ioctl.h>

#define AP3216C_GET_IR      _IOR('D',0,unsigned short)
#define AP3216C_GET_ALS     _IOR('D',1,unsigned short)
#define AP3216C_GET_PS      _IOR('D',2,unsigned short)
int main(void)
{
    int i,fd;
    int r1,r2,r3;
    unsigned short ir,als,ps;

    fd=open("/dev/ap3216c",O_RDWR);
    if(fd<0)
    {
        perror("open error");
        exit(EXIT_FAILURE);
    }

    for(i=0;i<10;i++)
    {
        r1=ioctl(fd,AP3216C_GET_IR,&ir);
        r2=ioctl(fd,AP3216C_GET_ALS,&als);
        r3=ioctl(fd,AP3216C_GET_PS,&ps);
        printf("<%d> ir=%u als=%u ps=%u (ret %d %d %d)\n",i+1,ir,als,ps,r1,r2,r3);
        fflush(stdout);
        sleep(2);
    }

    close(fd);
    return 0;
}