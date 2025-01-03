/*********************************************************************************
 *      Copyright:  (C) 2024 LingYun<lingyun@email.com>
 *                  All rights reserved.
 *
 *       Filename:  server.c
 *    Description:  This file is server main function
 *                 
 *        Version:  1.0.0(23/12/24)
 *         Author:  LingYun <lingyun@email.com>
 *      ChangeLog:  1, Release initial version on "23/12/24 22:15:45"
 *                 
 ********************************************************************************/
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <errno.h>
#include <ctype.h>
#include <time.h>
#include <pthread.h>
#include <getopt.h>
#include <libgen.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <sys/select.h>
#include <netinet/in.h>
#include <sys/epoll.h>
#include <sys/resource.h>

#define MAX_EVENTS      512
#define ARRAY_SIZE(x)   (sizeof(x)/sizeof(x[0]))

static  inline  void msleep(unsigned long ms);
static  inline  void print_usage(char *progname);
int     socket_server_init(char *listen_ip,int listen_port);
void    set_socket_rlimit(void);

struct temp_int
{
    float temp;
    char str[20];
};

static  inline void print_usage(char *progname)
{
    printf("Usage: %s [OPTION] ...\n",progname);

    printf(" %s is a socket server program , which used to verify client and echo back striing from it\n",progname);
    printf("\nMandatory arguments to long options are mandatory for short options too:\n");
    printf("-b[daemon ]  set program running on background\n");
    printf("-p(port   ): specify server listen port.\n");
    printf("-h(help   ): print this help information.\n");

    printf("\nExample: %s -b -p 8900\n", progname);

    return;
}

int main(int argc, char **argv)
{
    int             listenfd,connfd;
    int             serv_port =0;
    int             daemon_run =0;
    char            *progname = NULL;
    int             opt;
    fd_set          rdset;
    int             rv;
    int             i,j;
    int             found;
    int             maxfd=0;
    char            buf[1024];
    
    int             epollfd;
    struct  epoll_event event;
    struct  epoll_event event_array[MAX_EVENTS];
    int     events;
    struct temp_int *st=&buf;
    char *abc = " ";
    char *str1;
    char str2[20];
    struct option long_options[] = {
       {"daemon",no_argument,NULL,'b'},
       {"port", required_argument, NULL, 'p'},
        {"help", no_argument, NULL, 'h'},
        {NULL, 0, NULL, 0}};

    progname = basename(argv[0]);

    while ((opt = getopt_long(argc, argv, "bp:h", long_options, NULL)) != -1)
    {
        switch (opt)
        {
        case 'b':
            daemon_run = 1;
            break;

        case 'p':
            serv_port = atoi(optarg);
            break;
        case 'h':
            print_usage(argv[0]);
            return 0;
        }
    }
    if (!serv_port)
    {
        print_usage(progname);
        return -1;
    }

    set_socket_rlimit(); //set max socket count

    if((listenfd = socket_server_init(NULL,serv_port)) < 0 )
    {
        printf("ERROR: %s server listen on port %d failure\n",argv[0],serv_port);
        return -2;
    }
    printf("%s server start to listen on port %d \n",argv[0],serv_port);

    //run in background
    if(daemon_run )
    {
        daemon(0,0);
    }

    if((epollfd = epoll_create(MAX_EVENTS))<0)
    {
        printf("epoll_create() failure : %s\n",strerror(errno));
        return -3;
    }

    event.events = EPOLLIN;
    event.data.fd = listenfd;

    if(epoll_ctl(epollfd,EPOLL_CTL_ADD,listenfd,&event)< 0 )
    {
        printf("epoll_ctl() failure add listenfd ： %s\n",strerror(errno));
        return -4;
    }

    for(;;)
    {
        events = epoll_wait(epollfd, event_array, MAX_EVENTS, -1);
        if(events <0 )
        {
            printf("epoll failure: %s\n",strerror(errno));
            break;
        }
        else if(events == 0)
        {
            printf("epoll get timeout\n");
            continue;
        }

        //rv is the active events count
        for(i=0;i<events;i++)
        {
            if((event_array[i].events & EPOLLERR) || (event_array[i].events & EPOLLHUP))
            {
                printf("epoll_wait get error on fd[%d]: %s\n", event_array[i].data.fd, strerror(errno));
                epoll_ctl(epollfd,EPOLL_CTL_DEL,event_array[i].data.fd,NULL);
                close(event_array[i].data.fd);
            }

            //listen socket get new connect
            if(event_array[i].data.fd == listenfd)
            {
                if( (connfd=accept(listenfd, (struct sockaddr *)NULL, NULL)) < 0)
                {
                    printf("accept new client failure: %s\n", strerror(errno));
                    continue;
                }
                event.data.fd = connfd;
                //event.events = EPOLLIN|EPOLLET;
                event.events = EPOLLIN;
                if( epoll_ctl(epollfd, EPOLL_CTL_ADD, connfd, &event) < 0 )
                {
                    printf("epoll add client socket failure: %s\n", strerror(errno));
                    close(event_array[i].data.fd);
                    continue;
                }
                printf("epoll add new client socket[%d] ok.\n", connfd);
            }
            else//已经连接的client收到新的数据
            {
                if((rv = read(event_array[i].data.fd,buf,sizeof(buf))) <= 0)
                {
                    printf("socket[%d] read failure or get disconnect and will be removed .\n",
                    event_array[i].data.fd);
                    epoll_ctl(epollfd,EPOLL_CTL_DEL,event_array[i].data.fd,NULL);
                    close(event_array[i].data.fd);
                    continue;
                }
                else
                {
                    printf("socket[%d] read get %d byes data\n",event_array[i].data.fd,rv);
                    
                    printf("tempuree :%.2f  str : %s\n",st->temp,st->str);
                    // strncpy(str2,buf,sizeof(str2));
                    // str1 = strtok(str2, " ");
                    // st.temp=(float) atof(str1);
                    // str1 = strtok(NULL, " ");
                    // strncpy(st.str,str1,sizeof(st.str));
                    // printf("receive struct : st.tmep: %.2f st.str: %s \n",st.temp,st.str);
                    //printf("temp: %.2f  str:%s \n",st->temp,st->str);
                    //功能
                    // for(j=0;j<rv;j++)
                    //     buf[j] = toupper(buf[j]);
                    if(write(event_array[i].data.fd,buf,rv) < 0)
                    {
                        printf("socket[%d] write failure :%s\n",event_array[i].data.fd,strerror(errno));
                        epoll_ctl(epollfd,EPOLL_CTL_DEL,event_array[i].data.fd,NULL);
                        close(event_array[i].data.fd);
                    }
                }
            }

        }    //
    } //while   
        
CleanUp:
    close(listenfd);
    return 0;

    /*    if (argc <3)
       {
           printf("program usage: %s  [serverIP] [PORT]\n",argv[0]);
           return -1;
       }
       servip = argv[1];
       port = atoi(argv[2]); */

}

static inline void msleep(unsigned long ms)
{
    struct timeval tv;

    tv.tv_sec = ms/1000;
    tv.tv_usec = (ms%1000)*1000;

    select(0,NULL,NULL,NULL ,&tv);
}


int socket_server_init(char *listen_ip,int listen_port)
{
    struct sockaddr_in          servaddr;
    int                         rv=0;
    int                         on=1;
    int                         listenfd;

    if((listenfd = socket(AF_INET,SOCK_STREAM,0))<0)
    {
        printf("Create socket failure : %s\n", strerror(errno));
        return -1; 
    }
    
    setsockopt(listenfd,SOL_SOCKET,SO_REUSEADDR,&on,sizeof(on));
    
    memset(&servaddr, 0, sizeof(servaddr));
    servaddr.sin_family = AF_INET;
    servaddr.sin_port = htons(listen_port);

    if(!listen_ip)
    {
        servaddr.sin_addr.s_addr = htonl(INADDR_ANY);
    }
    else
    {
        if(inet_pton(AF_INET,listen_ip,&servaddr.sin_addr) <= 0)
        {
            printf("inet_pton() set listen IP addres failure .\n");
            rv=-2;
            goto CleanUp;
        }
       
    }
    if(bind(listenfd, (struct sockaddr *) &servaddr, sizeof(servaddr)) < 0)
    {
        printf("Use bind() to bind the TCP socket failure: %s\n", strerror(errno));
        rv = -3;
         goto CleanUp;
    }
    if(listen(listenfd, 13) < 0)
    {
        printf("Use bind() to bind the TCP socket failure: %s\n", strerror(errno));
        rv = -4;
        goto CleanUp;
    }
    CleanUp:
        if(rv<0)
            close(listenfd);
        else
        rv = listenfd;
    return rv;
    
}

void set_socket_rlimit(void)
{
   struct rlimit limit = {0};

    getrlimit(RLIMIT_NOFILE, &limit );
    limit.rlim_cur = limit.rlim_max;
    setrlimit(RLIMIT_NOFILE, &limit );
    printf("set socket open fd max count to %d\n", limit.rlim_max);
}

