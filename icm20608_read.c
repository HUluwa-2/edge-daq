#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <fcntl.h>
#include <errno.h>
#include <string.h>

int main(void)
{
    int i,fd;
    signed int databuf[7];

    fd=open("/dev/icm20608",O_RDWR);
    if(fd<0)
    {
        perror("open error");
        exit(EXIT_FAILURE);
    }

    for(i=0;i<10;i++)
    {
        errno=0;
        ssize_t ret=read(fd,databuf,sizeof(databuf));
        if(ret<0)
        {
            perror("read error");
        }
        int e=errno;
        float temp=((float)databuf[6]-25)/326.8 + 25;
        printf("<%d> ret=%zd errno=%d gx=%d gy=%d gz=%d ax=%d ay=%d az=%d traw=%d temp=%.1f\n",
        i+1,ret,ret<0?e:0,databuf[0],databuf[1],databuf[2],
        databuf[3],databuf[4],databuf[5],databuf[6],temp);
        fflush(stdout);
        sleep(3);
    }

    close(fd);
    exit(EXIT_SUCCESS);
}