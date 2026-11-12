#include <stdio.h>
#include <stdlib.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>
#include <sys/ioctl.h>
#include <sys/mman.h>
#include <linux/fb.h>
#include <string.h>
#include <signal.h>
#include "font8x16.h"

#define argb8888_to_rgb565(color)   ({\
    unsigned int temp=(color);\
    ((temp & 0xF80000UL)>>8)|\
    ((temp & 0xFC00UL)>>5)|\
    ((temp & 0xF8UL)>>3);\
})

static int width,height;
static unsigned short *screen_base=NULL;
static volatile sig_atomic_t g_stop=0;

static void lcd_draw_point(unsigned int x,unsigned int y,unsigned int color)
{
    unsigned short rgb565_color=argb8888_to_rgb565(color);

    if(x>width){
        x=width-1;
    }
    if(y>height)
    {
        y=height-1;
    }

    screen_base[y*width + x]=rgb565_color;
}

static void lcd_draw_line(unsigned int x,unsigned int y,int dir,unsigned int length,unsigned int color)
{
    unsigned short rgb565_color=argb8888_to_rgb565(color);
    unsigned int end;
    unsigned long temp;

    if(x>width){
        x=width-1;
    }
    if(y>height){
        y=height-1;
    }

    temp= y*width + x;
    if(dir){
        end=x+length-1;
        if(end>=width)
        {
            end=width-1;
        }
        for(;x<=end;x++,temp++){
            screen_base[temp]=rgb565_color;
        }
    }
    else{
        end=y+length-1;
        if(end>=height)
        {
            end=height-1;
        }
        for(;y<=end;y++,temp+=width){
            screen_base[temp]=rgb565_color;
        }
    }
}

static void lcd_draw_rectangle(unsigned int start_x,unsigned int start_y,unsigned int end_x,unsigned int end_y,unsigned int color){
    int x_len=end_x-start_x+1;
    int y_len=end_y-start_y+1;

    lcd_draw_line(start_x,start_y,1,x_len,color);
    lcd_draw_line(start_x,end_y,1,x_len,color);
    lcd_draw_line(start_x,start_y,0,y_len,color);
    lcd_draw_line(end_x,start_x,0,y_len,color);
}

static void lcd_fill(unsigned int start_x,unsigned int end_x,unsigned int start_y,unsigned int end_y,unsigned int color){
    unsigned short rgb565_color=argb8888_to_rgb565(color);
    unsigned long temp;
    unsigned int x;

    if(end_x>width){
        end_x=width-1;
    }
    if(end_y>height){
        end_y=height-1;
    }

    temp=start_y*width;
    for(;start_y<=end_y;start_y++,temp+=width){
        for(x=start_x;x<=end_x;x++){
            screen_base[temp+x]=rgb565_color;
        }
    }
}

static void lcd_draw_char(unsigned int x, unsigned int y, char ch, unsigned int color, int scale)
{
    if (ch < 32 || ch > 126) {
        return; 
    }
    const unsigned char *font_data = font8x16[ch - 32];

    for (int row = 0; row < 16; row++) {

        unsigned char line_bits = font_data[row]; 
        for (int col = 0; col < 8; col++) {
            if (line_bits & 0x80) {
                unsigned int draw_x = x + (col * scale);
                unsigned int draw_y = y + (row * scale);
                if (scale == 1) {
                    lcd_draw_point(draw_x, draw_y, color);
                } else {
                    lcd_fill(draw_x, 
                             draw_x + scale - 1, 
                             draw_y, 
                             draw_y + scale - 1, 
                             color);
                }
            }
            line_bits <<= 1; 
        }
    }
}

static void lcd_draw_string(unsigned int x, unsigned int y, const char *str, unsigned int color, int scale)
{
    if (str == NULL) {
        return;
    }
    while (*str != '\0') {

        lcd_draw_char(x, y, *str, color, scale);
        
        x += 8 * scale;
    
        str++;
    }
}

static void draw_dashboard(const char *temp_s,const char *light_s,const char *prox_s){
    lcd_fill(0,width-1,0,height-1,0x000000);
    lcd_draw_rectangle(1,1,10,10,0xFFFFFF);
    lcd_draw_string(40,40,"EdgeDAQ",0xFFFFFF,4);
    lcd_draw_string(40,200,temp_s,0x00FF00,3);
    lcd_draw_string(40,280,light_s,0x00FF00,3);
    lcd_draw_string(40,360,prox_s,0x00FF00,3);
}

static void INT_handler(int sig){
    g_stop=1;
}

int main(int argc,char *argv[])
{
    struct sigaction sig_int;
    sig_int.sa_flags=0;
    sig_int.sa_handler=INT_handler; 
    sigemptyset(&sig_int.sa_mask);
    sigaction(SIGINT,&sig_int,NULL);
   
    struct fb_fix_screeninfo fb_fix;
    struct fb_var_screeninfo fb_var;
    unsigned int screen_size;
    int fd;

    if(0>(fd=open("/dev/fb0",O_RDWR))){
        perror("open error");
        exit(EXIT_FAILURE);
    }

    ioctl(fd,FBIOGET_VSCREENINFO,&fb_var);
    ioctl(fd,FBIOGET_FSCREENINFO,&fb_fix);

    screen_size=fb_fix.line_length*fb_var.yres;
    width=fb_var.xres;
    height=fb_var.yres;

    screen_base=mmap(NULL,screen_size,PROT_READ|PROT_WRITE,MAP_SHARED,fd,0);
    if(MAP_FAILED==(void *)screen_base){
        perror("mmap error");
        close(fd);
        exit(EXIT_FAILURE);
    }

    char temp_str[64]="temp=--";
    char light_str[64]="light=--";
    char prox_str[64]="prox=--";
    
    while(!g_stop){
        FILE *fp=fopen("/tmp/edgedaq_data.log","r");

        if(!fp){
            strcpy(temp_str,"temp=--");
            strcpy(light_str,"light=--");
            strcpy(prox_str,"prox=--");
            draw_dashboard(temp_str,light_str,prox_str);
            sleep(1);
            continue;
        }

        fseek(fp,0,SEEK_END);
        long file_size=ftell(fp);
        long read_start=(file_size>4096)?(file_size-4096):0;
        fseek(fp,read_start,SEEK_SET);

        char buf[4097];
        size_t bytes=fread(buf,1,4096,fp);
        buf[bytes]='\0';
        fclose(fp);

        char *line=strtok(buf,"\n");
        while(line){
            char *p;
            if((p=strstr(line,"temp seq="))){
                char *val = strstr(p,"value=");
                if(val){
                    snprintf(temp_str,sizeof(temp_str),"temp=%s",val+6);
                }
            }
            else if((p=strstr(line,"light seq="))){
                char *val = strstr(p,"value=");
                if(val){
                    snprintf(light_str,sizeof(light_str),"light=%s",val+6);
                }
            }
            else if((p=strstr(line,"prox seq="))){
                char *val = strstr(p,"value=");
                if(val){
                    snprintf(prox_str,sizeof(prox_str),"prox=%s",val+6);
                }
            }
            line = strtok(NULL,"\n");
        }
        draw_dashboard(temp_str,light_str,prox_str);

        sleep(1);
    }

    lcd_fill(0,width-1,0,height-1,0x000000);

    munmap(screen_base,screen_size);
    close(fd);
    printf("[LCD]监控界面已退出\n");
    exit(EXIT_FAILURE);
}