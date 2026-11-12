#include <stdio.h>
#include <stdlib.h>
#include <fcntl.h>
#include <unistd.h>
#include <string.h>
#include <sys/ioctl.h>
#include <sys/mman.h>
#include <linux/fb.h>

int main(void)
{
    int fd=open("/dev/fb0",O_RDWR);
    if(fd<0){
        perror("open error");
        exit(EXIT_FAILURE);
    }

    struct fb_var_screeninfo vinfo;
    struct fb_fix_screeninfo finfo;
    ioctl(fd,FBIOGET_VSCREENINFO,&vinfo);
    ioctl(fd,FBIOGET_FSCREENINFO,&finfo);
    printf("xres=%d yres=%d bpp=%d line_length=%d smem_len=%d\n",
    vinfo.xres,vinfo.yres,vinfo.bits_per_pixel,
    finfo.line_length,finfo.smem_len);

    unsigned char *fb=mmap(NULL,finfo.smem_len,PROT_READ|PROT_WRITE,MAP_SHARED,fd,0);

    if(fb==MAP_FAILED){
        perror("mmap error");
        exit(EXIT_FAILURE);
    }

    int x,y;
    for(y=0;y<(int)vinfo.yres;y++){
        for(x=0;x<(int)vinfo.xres;x++){
            long offset = y*finfo.line_length + x*(vinfo.bits_per_pixel/8);
            unsigned int color;
            if (vinfo.bits_per_pixel == 16) {
                unsigned short c16 = (y < (int)vinfo.yres / 3) ? 0xF800 :
                                     (y < (int)vinfo.yres * 2 / 3) ? 0x07E0 : 0x001F;
                *(unsigned short *)(fb + offset) = c16;
                continue;
            }
            color = (y < (int)vinfo.yres / 3) ? 0x00FF0000 :
                    (y < (int)vinfo.yres * 2 / 3) ? 0x0000FF00 : 0x000000FF;
            *(unsigned int *)(fb + offset) = color;
        }
    }
    printf("filled. Ctrl+C 退出\n");
    pause();
    munmap(fb, finfo.smem_len);
    close(fd);
    return 0;
}