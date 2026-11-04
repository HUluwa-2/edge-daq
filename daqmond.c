#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <fcntl.h>
#include <sys/stat.h>
#include <time.h>
#include <sys/wait.h>
#include <signal.h>
void cread_daemon(){
    pid_t pid;
    pid=fork();
    if(pid<0){ perror("fork error"); exit(EXIT_FAILURE); }
    else if(pid>0){ printf("father process exit\n"); exit(EXIT_SUCCESS); }
    if(setsid()<0){ perror("setsid error"); exit(EXIT_FAILURE); }
    pid=fork();
    if(pid<0){ perror("fork error"); exit(EXIT_FAILURE); }
    if(pid>0){ printf("son process exit\n"); exit(EXIT_SUCCESS); }
    chdir("/");
    umask(0);
    for(int i=0;i<sysconf(_SC_OPEN_MAX);i++) close(i);
    open("/dev/null",O_RDWR);
    dup(0);
    dup(0);
}
volatile sig_atomic_t g_stop=0,g_reload=0;
static void sig_handler(int sig)
{
    if(sig==SIGTERM||sig==SIGINT){ g_stop=1; }
    if(sig==SIGHUP){ g_reload=1; }
}
int main(void)
{
    cread_daemon();
    time_t rawtime; struct tm *timeinfo; char buffer[80];
    int status; pid_t pid;
    struct sigaction sig={0};
    sig.sa_flags=0;
    sig.sa_handler=sig_handler;
    sigemptyset(&sig.sa_mask);
    if(0>(sigaction(SIGTERM,&sig,NULL))){ exit(EXIT_FAILURE); }
    if(0>(sigaction(SIGINT,&sig,NULL))){ exit(EXIT_FAILURE); }
    if(0>(sigaction(SIGHUP,&sig,NULL))){ exit(EXIT_FAILURE); }
    FILE *fp=fopen("/tmp/daqmond.log","a+");
    if(fp==NULL){ exit(EXIT_FAILURE); }
    for(int i=0;i<3;i++)
    {
        pid=fork();
        if(pid<0){ exit(EXIT_FAILURE); }
        else if(pid==0){
            if(i==0){ if(0>execlp("ls","ls","-a","-l",NULL)){ _exit(127); } }
            if(i==1){ char *a[]={"data",NULL}; if(0>execvp("data",a)){ _exit(127); } }
            if(i==2){ if(0>execl("/bin/ps","ps",NULL)){ _exit(127); } }
        }
    }
    while ((pid = waitpid(-1, &status, 0)) > 0){
        if(WIFEXITED(status)){
            time(&rawtime); timeinfo=localtime(&rawtime);
            strftime(buffer,sizeof(buffer),"%Y-%m-%d %H:%M:%S ",timeinfo);
            fprintf(fp,"%s",buffer);
            fprintf(fp,"parent process pid %d has reaped child process %d,exitstatus<%d>\n",getpid(),pid,WEXITSTATUS(status));
            fflush(fp);
        }
        else{
            time(&rawtime); timeinfo=localtime(&rawtime);
            strftime(buffer,sizeof(buffer),"%Y-%m-%d %H:%M:%S ",timeinfo);
            fprintf(fp,"%s",buffer);
            fprintf(fp,"child %d killed sig %d\n",pid,WTERMSIG(status));
            fflush(fp);
        }
    }
    while(!g_stop)
    {
        if (g_reload) {
            g_reload = 0;
            fclose(fp);
            fp = fopen("/tmp/daqmond.log", "a+");
            if (fp == NULL) exit(EXIT_FAILURE);
            time(&rawtime); timeinfo=localtime(&rawtime);
            strftime(buffer,sizeof(buffer),"%Y-%m-%d %H:%M:%S ",timeinfo);
            fprintf(fp,"%s",buffer);
            fprintf(fp,"log reopened\n");
            fflush(fp);
        }
        time(&rawtime); timeinfo=localtime(&rawtime);
        strftime(buffer,sizeof(buffer),"%Y-%m-%d %H:%M:%S ",timeinfo);
        fprintf(fp,"%s\n",buffer);
        fflush(fp);
        sleep(5);
    }
    if(g_stop)
    {
        time(&rawtime); timeinfo=localtime(&rawtime);
        strftime(buffer,sizeof(buffer),"%Y-%m-%d %H:%M:%S ",timeinfo);
        fprintf(fp,"%s",buffer);
        fprintf(fp,"daqmond stopped\n");
        fflush(fp);
    }
    fclose(fp);
    return 0;
}
