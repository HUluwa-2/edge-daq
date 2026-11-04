#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>
#include <string.h>
#include <unistd.h>
#include <sys/types.h>

pthread_mutex_t mutex1,mutex2;

static void *thread1_start(void *arg)
{
    pthread_mutex_lock(&mutex1);
    printf("t1 got A\n");
    fflush(stdout);
    usleep(100000);
    pthread_mutex_lock(&mutex2);

    pthread_mutex_unlock(&mutex2);
    pthread_mutex_unlock(&mutex1);

    return NULL;
}

static void *thread2_start(void *arg)
{
    pthread_mutex_lock(&mutex2);
    printf("t2 got B\n");
    fflush(stdout);
    usleep(100000);
    pthread_mutex_lock(&mutex1);

    pthread_mutex_unlock(&mutex1);
    pthread_mutex_unlock(&mutex2);

    return NULL;
}

int main(int argc,char *argv[])
{
    pthread_t tid1,tid2;
    int ret;

    pthread_mutex_init(&mutex1,NULL);
    pthread_mutex_init(&mutex2,NULL);

    ret=pthread_create(&tid1,NULL,thread1_start,NULL);
    if(ret)
    {
        fprintf(stderr,"pthread_create error:%s\n",strerror(ret));
        exit(EXIT_FAILURE);
    }

    ret=pthread_create(&tid2,NULL,thread2_start,NULL);
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

    pthread_mutex_destroy(&mutex1);
    pthread_mutex_destroy(&mutex2);

    exit(EXIT_SUCCESS);
}
