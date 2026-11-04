#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>
#include <string.h>
#include <unistd.h>
#include <sys/types.h>

int count=0;
pthread_mutex_t mutex;
static void *new_thread_start(void *arg)
{
    int loops,temp;
    loops=*((int *)arg);
    for(int i=0;i<loops;i++)
    {
        pthread_mutex_lock(&mutex);
        temp=count;
        temp++;
        count=temp;
        pthread_mutex_unlock(&mutex);
    }
    pthread_exit(NULL);
}

int main(int argc,char *argv[])
{
    int loops=1500000;
    pthread_t tid1,tid2;
    int ret;
    pthread_mutex_init(&mutex,NULL);
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

    pthread_mutex_destroy(&mutex);
    printf("count:%d\n",count);
    exit(EXIT_SUCCESS);
}
