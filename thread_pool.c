#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>
#include <unistd.h>

typedef struct {
    void (*function)(void *);
    void *arg;
}Task;

typedef struct{
    Task *task_queue;
    int queue_size;
    int head,tail,count;

    pthread_t *threads;
    int num_threads;

    pthread_mutex_t lock;
    pthread_cond_t notify;
    int shutdown;
}ThreadPool;

void *thread_worker(void *threadpool){
    
        ThreadPool *pool=(ThreadPool *)threadpool;
    while(1){

        pthread_mutex_lock(&(pool->lock));

        while((pool->count==0)&&(!pool->shutdown)){
            pthread_cond_wait(&(pool->notify),&(pool->lock));
        }

        if(pool->shutdown && pool->count==0){
            pthread_mutex_unlock(&(pool->lock));
            pthread_exit(NULL);
        }

        Task task = pool->task_queue[pool->head];
        pool->head = (pool->head +1)% pool->queue_size;
        pool->count --;

        pthread_mutex_unlock(&(pool->lock));

        (*(task.function))(task.arg);
    }
    return NULL;
}

ThreadPool* thread_pool_init(int num_threads,int queue_size){
    ThreadPool *pool=(ThreadPool *)malloc(sizeof(ThreadPool));
    pool->num_threads=num_threads;
    pool->queue_size=queue_size;
    pool->head=pool->tail=pool->count=0;
    pool->shutdown=0;

    pool->threads=(pthread_t *)malloc(sizeof(pthread_t)*num_threads);
    pool->task_queue=(Task *)malloc((sizeof(Task)*queue_size));

    pthread_mutex_init(&(pool->lock),NULL);
    pthread_cond_init(&(pool->notify),NULL);

    for(int i=0;i<num_threads;i++)
    {
        pthread_create(&(pool->threads[i]),NULL,thread_worker,(void *)pool);
    }

    return pool;
}

int thread_pool_add(ThreadPool *pool,void (*function)(void *),void *arg)
{
    pthread_mutex_lock(&(pool->lock));

    if(pool->count==pool->queue_size||pool->shutdown)
    {
        pthread_mutex_unlock(&(pool->lock));
        return -1;
    }

    pool->task_queue[pool->tail].function=function;
    pool->task_queue[pool->tail].arg=arg;
    pool->tail=(pool->tail +1 )% pool->queue_size;
    pool->count ++;

    pthread_cond_signal(&(pool->notify));
    pthread_mutex_unlock(&(pool->lock));

    return 0;
}

void thread_pool_destroy(ThreadPool *pool)
{
    pthread_mutex_lock(&(pool->lock));
    pool->shutdown=1;

    pthread_cond_broadcast(&(pool->notify));
    pthread_mutex_unlock(&(pool->lock));

    for(int i=0;i<pool->num_threads;i++)
    {
        pthread_join(pool->threads[i],NULL);
    }

    free(pool->task_queue);
    free(pool->threads);
    pthread_mutex_destroy(&(pool->lock));
    pthread_cond_destroy(&(pool->notify));
    free(pool);
}

void printf_task(void *arg)
{
    int task_id=*(int *)arg;
    pthread_t thread_id=pthread_self();

    printf("任务%2d 被线程 %lx 执行\n",task_id,(unsigned long)thread_id);

    usleep(100000);
}

int main()
{
    ThreadPool *pool=thread_pool_init(8,30);

    int task_ids[20];

    for(int i=0;i<20;i++)
    {
        task_ids[i]=i+1;
        thread_pool_add(pool,printf_task,(void*)&task_ids[i]);
    }

    printf("-------投递20个任务完毕--------\n");
    thread_pool_destroy(pool);

    printf("-------线程池已摧毁--------\n");

    return 0;
}