#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>
#include <string.h>
#include <unistd.h>
#include <sys/types.h>
#include <unistd.h>

int count=0;
static void *new_thread_start(void *arg)
{
    int loops,temp;
    loops=*((int *)arg);
    for(int i=0;i<loops;i++)
    {
        temp=count;
        temp++;
        count=temp;
    }
    pthread_exit(NULL);
}

int main(int argc,char *argv[])
{
    int loops=1500000;
    pthread_t tid1,tid2;
    int ret;

    ret=pthread_create(&tid1,NULL,new_thread_start,&loops);
    if(ret)
    {
        fprintf(stderr,"pthread_create error:%s\n",strerror(ret));
        exit(EXIT_FAILURE);
    }

    ret=pthread_create(&tid2,NULL,new_thread_start,&loops);
    if(ret)
    {
        fprintf(stderr,"pthread_create error:%s\n",strerror(ret));
        exit(EXIT_FAILURE);
    }

    ret=pthread_join(tid1,NULL);
    if(ret)
    {
        fprintf(stderr,"pthread_join error:%s\n",strerror(ret));
        exit(EXIT_FAILURE);
    }

    ret=pthread_join(tid2,NULL);
    if(ret)
    {
        fprintf(stderr,"pthread_join error:%s\n",strerror(ret));
        exit(EXIT_FAILURE);
    }

    printf("count:%d\n",count);
    exit(EXIT_SUCCESS);
}