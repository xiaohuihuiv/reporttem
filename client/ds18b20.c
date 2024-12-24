#include <sys/types.h>
#include <sys/stat.h>
#include <dirent.h>
#include <fcntl.h>
#include <unistd.h>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <errno.h>
#include "ds18b20.h"

int  temperature(float *temp);


int temperature(float *temp)
{
    int                     fd=-1;
    char                    buf[129] ;
    char                    *ptr=NULL;
    DIR                     *dirp=NULL;
    struct     dirent       *direntp = NULL;    
    char                    w1_path[64]="/sys/bus/w1/devices/";
    char                    chip_sn[32];   
    char                    ds18b20_path[64];                  
    int                     found = 0;

    dirp=opendir(w1_path);
    if (dirp == NULL)
    {
        printf("open folder %s failure: %s\n",w1_path,strerror(errno));
        return -1;
    }

    while ( NULL != (direntp=readdir(dirp)) )
    {   
        if (strstr(direntp->d_name,"28-"))
        {
            strncpy(chip_sn,direntp->d_name,sizeof(chip_sn));
            found =1;
        }
        
        //printf("fliename: %s\n",direntp->d_name);

    }
    if(!found)
    {
        printf("can not find ds18b20 chipset\n");
        return -2;
    }
    snprintf(ds18b20_path,sizeof(ds18b20_path),"%s/%s/w1_slave",w1_path,chip_sn);
    //printf("ds18b20_path:%s\n",ds18b20_path);

    closedir(dirp);
    
    
    fd = open(ds18b20_path,O_RDONLY);  
    if (fd < 0)
    {
        printf("open file failure: %s\n",strerror(errno));
        return -3;
         
    }
 
    
    memset(buf,0,sizeof(buf));
    
    read(fd,buf,sizeof(buf)) ;
    //printf("buf: %s\n",buf);

    ptr = strstr(buf,"t=");

    if (ptr == (void *) 0)
    {
        printf("can not find t= string");
        return -4; 
    }


    ptr += 2;
    //printf("ptr : %s\n",ptr);

    *temp = atof(ptr);
    *temp = *temp / 1000; 
    //printf("temprature: %f\n",*temp);

    close(fd);   
    return 0;     
}