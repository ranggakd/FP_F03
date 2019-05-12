#include<stdlib.h>
#include<stdio.h>
#include<string.h>
#include<unistd.h>
#include<pthread.h>
#include<time.h>
#include<sys/types.h>
#include<sys/stat.h>
#include<sys/wait.h>
#include<fcntl.h>
#include<errno.h>
#include<syslog.h>

//makro
#define new(x) (x*)malloc(sizeof(x))
#define maks 1000

//struct data
struct pipe_time
{
    int pminute;
    int phour;
    int pday;
    int pmonth;
    int pdate;
};
struct cron_line
{
    int cminute[60];
    int chour[24];
    int cday[32];
    int cmonth[13];
    int cdate[7];
    char cs[maks];
};
struct cron_data
{
    struct cron_line *cl;
    int flag;
    pthread_t *tid;
    char rawLine[maks];
};

//global
static const char *cron = "/home/rak/FP_SISOP19_F03/crontab.data";
// int lineFile;
pthread_t listThreads[maks];
struct cron_data* cron_thread[maks];

//fungsi non thread
void initMalloc()
{
    for (int i = 0; i < maks; i++)
    {
        struct cron_data* cdata = new(struct cron_data);
        cron_thread[i]=cdata;
    }
}

int execCommand(char argv[])
{
    int res;
    res = system(argv);
    // printf("c %d\n",res);
    if (res<0)
        return -1;
    return 0;
}

int execScript(char argv[])
{
    int res;
    char sy[maks];
    sprintf(sy,"bash %s",argv);
    res = system(sy);
    // printf("s %d\n",res);
    if (res<0)
        return -1;
    return 0;
}

void assignNumber(struct cron_line* cl, char argv1[], char argv2[], char argv3[],
char argv4[], char argv5[])
{
    //minute
    if (strcmp(argv1,"*")==0)
    {
        // printf("masuk\n");
        for (int i = 0; i < 60; i++)
            cl->cminute[i]=1;
    }
    else
        cl->cminute[atoi(argv1)]=1;
    //hour
    if (strcmp(argv2,"*")==0)
    {
        for (int i = 0; i < 24; i++)
            cl->chour[i]=1;
    }
    else
        cl->chour[atoi(argv2)]=1;
    //day
    if (strcmp(argv3,"*")==0)
    {
        for (int i = 0; i < 32; i++)
            cl->cday[i]=1;
        cl->cday[0]=0;
    }
    else
        cl->cday[atoi(argv3)]=1;
    //month
    if (strcmp(argv4,"*")==0)
    {
        for (int i = 0; i < 13; i++)
            cl->cmonth[i]=1;
        cl->cmonth[0]=0;
    }
    else
        cl->cmonth[atoi(argv4)]=1;
    //date
    if (strcmp(argv5,"*")==0)
    {
        for (int i = 0; i < 7; i++)
            cl->cdate[i]=1;
    }
    else
        cl->cdate[atoi(argv5)]=1;    
}

struct cron_line* parsingLine(char argv[])
{
    struct cron_line* cline=new(struct cron_line);
    char cmin[10], cho[10], cda[10], cmon[10], cdat[10];
    sscanf(argv,"%s %s %s %s %s %[^'\n']",cmin,cho,cda,cmon,cdat,cline->cs);
    // printf("%s %s %s %s %s %s\n",cmin,cho,cda,cmon,cdat,cline->cs);
    assignNumber(cline,cmin,cho,cda,cmon,cdat);
    // for (int i = 0; i < 60; i++)
    // {
    //     printf("%d ",cline->cminute[i]);
    // }
    // printf("m\n");
    // for (int i = 0; i < 24; i++)
    // {
    //     printf("%d ",cline->chour[i]);
    // }
    // printf("h\n");
    // for (int i = 0; i < 32; i++)
    // {
    //     printf("%d ",cline->cday[i]);
    // }
    // printf("d\n");
    // for (int i = 0; i < 13; i++)
    // {
    //     printf("%d ",cline->cmonth[i]);
    // }
    // printf("mo\n");
    // for (int i = 0; i < 7; i++)
    // {
    //     printf("%d ",cline->cdate[i]);
    // }
    // printf("da\n");
    return cline;
}

int getAvailableCronData()
{
    int counter=0;
    for (int i = 0; i < maks; i++)
    {
        if (cron_thread[i]->tid==NULL)
            counter++;
        // if (cron_thread[i].tid==0)
        //     counter++;
    }
    return counter;
}

int getAvailableIndexThread()
{
    for (int i = 0; i < maks; i++)
    {
        if (listThreads[i]==0)
            return i;
    }
    return -1;
}

struct pipe_time* timeNow(int flag)
{
    time_t t = time(NULL);
    struct tm tm = *localtime(&t);
    //waktu now+1 minute
    if (flag==1)
    {
        tm.tm_sec+=60;
        mktime(&tm);
    }
    //waktu now
    struct pipe_time *ptime = new(struct pipe_time);
    ptime->pminute=tm.tm_min;
    ptime->phour=tm.tm_hour;
    ptime->pday=tm.tm_mday;
    ptime->pmonth=tm.tm_mon+1;
    ptime->pdate=tm.tm_wday;
    return ptime;
}

int isTimeInCronLine(struct cron_line *cline, struct pipe_time *ptime)
{
    int flag=0;
    if (cline->cminute[ptime->pminute]==1 && cline->chour[ptime->phour]==1 &&
    cline->cday[ptime->pday]==1 && cline->cmonth[ptime->pmonth]==1 &&
    cline->cdate[ptime->pdate]==1)
        return 1;
    return 0;
}

int timeCompare(struct pipe_time *p1, struct pipe_time *p2)
{
    if (p1->pminute==p2->pminute && p1->phour==p2->phour &&
    p1->pday==p2->pday && p1->pmonth==p2->pmonth && p1->pdate==p2->pdate)
        return 1;
    return 0;
}

//fungsi thread
void* execThread(void* arv)
{
    struct cron_data* cdata=(struct cron_data*)arv;
    int f=0;
    struct pipe_time* nextOneMinute = new(struct pipe_time);
    while (1)
    {
        struct pipe_time* now = new(struct pipe_time);
        now=timeNow(0);
        // printf("if0 f %d now %d %d %d %d %d\tnext %d %d %d %d %d\n",f,now->pminute,now->phour,now->pday,now->pmonth,now->pdate,
        //     nextOneMinute->pminute,nextOneMinute->phour,nextOneMinute->pday,
        //     nextOneMinute->pmonth,nextOneMinute->pdate);
        //cek apakah waktu sekarang ada di cron line
        if (f==0 && isTimeInCronLine(cdata->cl,now)==1)
        {
            //cek command atau script
            if (strstr(cdata->cl->cs," ")==NULL)
                execScript(cdata->cl->cs);
            else
                execCommand(cdata->cl->cs);
            f=1;
            nextOneMinute=timeNow(1);
            // printf("if1 f %d now %d %d %d %d %d\tnext %d %d %d %d %d\n",f,now->pminute,now->phour,now->pday,now->pmonth,now->pdate,
            // nextOneMinute->pminute,nextOneMinute->phour,nextOneMinute->pday,
            // nextOneMinute->pmonth,nextOneMinute->pdate);
        }
        //untuk membuat flag berubah dan sekali exec saja
        if (timeCompare(now,nextOneMinute)==1)
        {
            f=0;
            nextOneMinute=timeNow(1);
        }
        // printf("if2 f %d now %d %d %d %d %d\tnext %d %d %d %d %d\n",f,now->pminute,now->phour,now->pday,now->pmonth,now->pdate,
        //     nextOneMinute->pminute,nextOneMinute->phour,nextOneMinute->pday,
        //     nextOneMinute->pmonth,nextOneMinute->pdate);
        // printf("ti %lu\n",pthread_self());
        free(now);
    }
    free(nextOneMinute);
}

void insertTempToCronData(char temp[][maks], int tmp)
{
    for (int i = 0; i < maks; i++)
    {
        printf("ga %d tmp %d\n",getAvailableCronData(), tmp);
        if (getAvailableCronData()-tmp<0 || tmp<=0)
            break;
        if (cron_thread[i]->tid==NULL)
        {
            struct cron_line *cline = new(struct cron_line);
            cline=parsingLine(temp[tmp-1]);
            cron_thread[i]->cl=cline;
            sprintf(cron_thread[i]->rawLine,"%s",temp[tmp-1]);
            printf("geait %d\n",getAvailableIndexThread());
            cron_thread[i]->tid=&listThreads[getAvailableIndexThread()];
            pthread_create(&listThreads[getAvailableIndexThread()],NULL,execThread,(void*)cron_thread[i]);
            tmp--;
        }
    }   
}

void setAllZeroFlag()
{
    for (int i = 0; i < maks; i++)
        cron_thread[i]->flag=0;
}

void setZeroListThreads(pthread_t *t)
{
    for (int i = 0; i < maks; i++)
    {
        if (t == &listThreads[i])
        {
            listThreads[i]=0;
            return;
        }
    }
}

void cancelAllZeroFlagThread()
{
    for (int i = 0; i < maks; i++)
    {
        if (cron_thread[i]->flag==0 && cron_thread[i]->tid!=NULL)
        {//cancel thread dan hapus data
            // pthread_cancel(*(cron_thread[i]->tid));
            pthread_cancel((*cron_thread[i]->tid));
            free(cron_thread[i]->cl); //? atau set 0 semua
            cron_thread[i]->flag=0;
            // free(cron_thread[i]->tid); //ini dari array thread
            setZeroListThreads(cron_thread[i]->tid);
            cron_thread[i]->tid=NULL;
            cron_thread[i]->rawLine[0]='\0';
        }
    }
}

int countLine()
{
    int counter=0;
    for (int i = 0; i < maks; i++)
    {
        if (strlen(cron_thread[i]->rawLine)!=0)
            counter++;
    }
    return counter;
}

int searchLine(char argv[])
{
    for (int i = 0; i < maks; i++)
    {
        if (strcmp(argv,cron_thread[i]->rawLine)==0)
            return i;
    }
    return -1;
}

void bacaFile()
{
    //baca tiap line pada file
    FILE* crontab=fopen(cron,"r");
    char *li = new(char);
    size_t *n = new(size_t);
    int counterLine=0;
    int tmp=0;
    char temp[maks][maks];
    //iter untuk hitung jumlah line dan cari ada command yang beda
    while (getline(&li,n,crontab)!=-1)
    {
        counterLine++;
        char ok[maks];
        sprintf(ok,"%s",li);
        //jika commandLine di file = commandLine di struct
        int fix;
        if ((fix=searchLine(ok))!=-1) //berhasil >=0 gagal -1
        {
            printf("fix %d\n",fix);
            cron_thread[fix]->flag=1;
            continue;
        }
        else
        {
            sprintf(temp[tmp],"%s",ok);
            tmp++;
            printf("temp %s\n",temp[tmp]);
        }
    }
    //cek apakah jumlah line di file sama dengan line di struct
    if (counterLine!=countLine())
    {
        printf("cl %d fung cl %d tmp %d\n",counterLine,countLine(),tmp);
        //cek apakah ada commandLine baru
        if (tmp==0)
        {//hapus semua data dan pthread_cancel [DELETE CASE]
            cancelAllZeroFlagThread();
            setAllZeroFlag();
        }
        else
        {//tambahkan temp ke struct dan exec thread [ADD CASE]
            insertTempToCronData(temp,tmp);
            setAllZeroFlag();
        }
    }
    else
    {
        if (tmp!=0)
        {//hapus yg lama dan tambahkan temp baru ke struct dan exec thread [UPDATE CASE]
            cancelAllZeroFlagThread();
            insertTempToCronData(temp,tmp);
            setAllZeroFlag();            
        }
    } 
    free(li);
    free(n);
    fclose(crontab);
}

int main()
{
    //template daemon
    pid_t pid, sid;
    pid=fork();
    if (pid<0) exit(EXIT_FAILURE);
    if (pid>0) exit(EXIT_SUCCESS);
    umask(0);
    sid=setsid();
    if (sid<0) exit(EXIT_FAILURE);
    if ((chdir("/home/rak/FP_SISOP19_F03/"))<0) exit(EXIT_FAILURE);
    close(STDIN_FILENO);
    close(STDOUT_FILENO);
    close(STDERR_FILENO);

    //main program kudu di fork daemonnya
    int status;
    pid=fork();
    if (pid==0)
    {//child dari daemon
        initMalloc();
        int flag=0;
        while (1)
        {
            //cek waktu modification
            struct stat *st = new(struct stat);
            stat(cron,st);
            time_t tnow = time(NULL);
            // struct tm tm = *localtime(&t);
            if (difftime(tnow,st->st_mtime)==0 && flag==0)
            {
                //baca file kalo ada modif time terbaru
                printf("diff %f\n",difftime(tnow,st->st_mtime));
                bacaFile();
                flag=1;
            }
            if (difftime(tnow,st->st_mtime)!=0)
                flag=0;
            free(st);
            // sleep(2);
        }
    }
    // else
    // //parent
    //     while ((wait(&status))>0);
    exit(EXIT_SUCCESS);
}