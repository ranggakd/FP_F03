# FP_SISOP19_F03
Keperluan final project laboratorium Sistem Operasi 2019

<center>

![image](fp.jpg "fp")

</center>

---

## Soal

Buatlah program C yang menyerupai crontab menggunakan daemon dan thread. Ada sebuah file crontab.data untuk menyimpan config dari crontab. Setiap ada perubahan file tersebut maka secara otomatis program menjalankan config yang sesuai dengan perubahan tersebut tanpa perlu diberhentikan. Config hanya sebatas * dan 0-9 (tidak perlu /,- dan yang lainnya)

## Jawab

Makro
```c
#define new(x) (x*)malloc(sizeof(x))
#define maks 1000
```

Struktur Data yang dipakai
```c
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
```

Global Variabel
```c
static const char *cron = "/home/rak/FP_SISOP19_F03/crontab.data";
pthread_t listThreads[maks];
struct cron_data* cron_thread[maks];
```

Dari main menggunakan template daemon kemudian main program dilakukan fork daemon. Fungsi initMalloc guna mengalokasikan memori array dari pointer struct global. Loop while untuk memicu bacaFile crontab.data apabila waktu modifikasi tepat berselisih 0 dengan program.
```c
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
```

Fungsi bacaFile melakukan pembacaan line per line untuk melakukan tiga tugas yakni, ADD, UPDATE dan DELETE command line yang ada pada struct global.
```c
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
```

Fungsi cancelAllZeroFlagThread untuk membatalkan seluruh command line yang sudah tak tertera di file lagi, namun masih ada pada struct.
```c
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
```

Fungsi insertTempToCronData untuk memasukkan command line yang baru kedalam struct global dan mengeksekusikannya pada thread.
```c
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
```

Fungsi execThread adalah fungsi blok thread guna mengeksekusi command / script pada waktu yang telah diurai dari command line yang ditentukan.
```c
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
```

Fungsi lain merupakan pelengkap dan sebagai dokumentasi bisa lebih baik.