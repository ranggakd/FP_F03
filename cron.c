#include<stdlib.h>
#include<stdio.h>
#include<string.h>

//global
static const char *cron = "/home/rak/Downloads";

int execCommand(char argv[])
{
    int res;
    res = system(argv);
    printf("c %d\n",res);
    if (res<0)
        return -1;
    return 0;
}

int execScript(char argv[])
{
    int res;
    char sy[1000];
    sprintf(sy,"bash %s",argv);
    res = system(sy);
    printf("s %d\n",res);
    if (res<0)
        return -1;
    return 0;
}

int main()
{
    //script
    char d[]={"/mnt/d/Repository/FP_F03/crontab.data"};
    execScript(d);
    // system("bash /mnt/d/Repository/FP_F03/crontab.data");
    // printf("lanjut\n");
    //command
    char u[]={"cp /mnt/d/Repository/FP_F03/fp.jpg /mnt/d/Repository/FP_F03/lalal"};
    execCommand(u);
    // system("cp /mnt/d/Repository/FP_F03/fp.jpg /mnt/d/Repository/FP_F03/lalal");
    // printf("ok");
    return 0;
}