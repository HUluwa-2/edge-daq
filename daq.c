#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>
#include <unistd.h>
#include <fcntl.h>
#include <sys/stat.h>
#include <time.h>
#include <string.h>
#include <signal.h>
#include <errno.h>

FILE *data_fp=NULL;
pthread_mutex_t file_lock=PTHREAD_MUTEX_INITIALIZER;
static volatile sig_atomic_t g_stop=0;
static volatile sig_atomic_t g_reload=0;

static void TERM_handler(int sig)
{
    g_stop=1;
}

static void HUP_handler(int sig)
{
    g_reload=1;
}

void create_daemon(){
    pid_t pid;

    pid=fork();
    if(pid<0){
        perror("fork error");
        exit(EXIT_FAILURE);
    }
    else if(pid>0){
        printf("father process exit\n");
        exit(EXIT_SUCCESS);
    }

    if(setsid()<0){
        perror("setsid error");
        exit(EXIT_FAILURE);
    }

    pid=fork();
    if(pid<0){
        perror("fork error");
        exit(EXIT_FAILURE);
    }
    if(pid>0){
        printf("son process exit\n");
        exit(EXIT_SUCCESS);
    }

    chdir("/");

    umask(0);

    for(int i=0;i<sysconf(_SC_OPEN_MAX);i++)
    {
        close(i);
    }

    open("/dev/null",O_RDWR);
    open("/tmp/daqmond.log", O_RDWR|O_CREAT|O_APPEND, 0644);
    dup(1);
}

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

void process_data(void *arg)
{
    char *line = (char*)arg;

    time_t rawtime;
    struct tm *timeinfo;
    char time_buf[64];
    time(&rawtime);
    timeinfo=localtime(&rawtime);
    strftime(time_buf,sizeof(time_buf),"%Y-%m-%d %H:%M:%S",timeinfo);

    char out_buf[1024];
    snprintf(out_buf,sizeof(out_buf),"[%s] %s\n",time_buf,line);

    pthread_mutex_lock(&file_lock);
    fputs(out_buf,data_fp);
    fflush(data_fp);
    pthread_mutex_unlock(&file_lock);

    free(line);
}

int main()
{
    create_daemon();
    setvbuf(stdout,NULL,_IOLBF,0);

    int n=0;
    struct sigaction sig_term;
    struct sigaction sig_hup;
    int ret;

    memset(&sig_term, 0, sizeof(sig_term));
    memset(&sig_hup, 0, sizeof(sig_hup));
    
    sig_term.sa_handler=TERM_handler;
    sig_term.sa_flags=0;
    sigemptyset(&sig_term.sa_mask);
    ret=sigaction(SIGTERM,&sig_term,NULL);
    if(ret==-1){
        perror("sigaction error");
        exit(EXIT_FAILURE);
    }

    sig_hup.sa_handler=HUP_handler;
    sig_hup.sa_flags=0;
    sigemptyset(&sig_hup.sa_mask);
    ret=sigaction(SIGHUP,&sig_hup,NULL);
    if(ret==-1){
        perror("sigaction error");
        exit(EXIT_FAILURE);
    }

    ThreadPool *pool=thread_pool_init(4,64);

    data_fp=fopen("/tmp/edgedaq_data.log","a");
    if(data_fp==NULL){
        perror("open error");
        exit(EXIT_FAILURE);
    }

    const char *fifo_path="/tmp/edgedaq.fifo";
    mkfifo(fifo_path,0666);

    printf("[Reader] 准备从管道读取数据...\n");
    int fd=open(fifo_path,O_RDONLY);
    if(fd<0){
        perror("open error");
        exit(EXIT_FAILURE);
    }

    char line_buf[4096];
    int buf_len=0;

    while(!g_stop)
    {
        if(g_reload)
        {
            g_reload=0;
            pthread_mutex_lock(&file_lock);
            if(data_fp){
                fclose(data_fp);
            }
            data_fp=fopen("/tmp/edgedaq_data.log","a");
            if(data_fp==NULL){
                perror("日志重开失败");
                exit(EXIT_FAILURE);
            }
            pthread_mutex_unlock(&file_lock);
            printf("收到SIGHUP信号,日志已重开\n");
        }
        ssize_t bytes=read(fd,line_buf+buf_len,sizeof(line_buf)-buf_len-1);
        if(bytes < 0) {
            if (errno == EINTR) {
                continue;
            } else {
                perror("read error");
                break; 
            }
        }
        else if(bytes == 0) {
            printf("写端已退出\n");
            break;
        }

        buf_len += bytes;

        while(1)
        {
            int newline_idx = -1;
            for(int i = 0;i < buf_len; i++)
            {
                if(line_buf[i]=='\n'){
                    newline_idx = i;
                    break;
                }
            }

            if(newline_idx == -1){
                break;
            }

            int line_size = newline_idx;
            char *task_arg = (char*)malloc(line_size + 1);
            if(task_arg){
                memcpy(task_arg,line_buf,line_size);
                task_arg[line_size]='\0';

                int r=thread_pool_add(pool,process_data,(void *)task_arg);
                if(r==0){
                    n++;
                }
                else{
                    free(task_arg);
                }
            }

            int remain = buf_len - (newline_idx + 1);
            if(remain > 0){
                memmove(line_buf,line_buf + newline_idx + 1,remain);
            }
            buf_len = remain;
        }
    }
    thread_pool_destroy(pool);
    close(fd);
    fclose(data_fp);

    if(g_stop){
        printf("RECEIVE SIGTERM,EXIT SUCCESS\n");
    }

    printf("共完成%d次任务\n",n);
}