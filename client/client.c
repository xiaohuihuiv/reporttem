/*********************************************************************************
 *      Copyright:  (C) 2024 LingYun<lingyun@email.com>
 *                  All rights reserved.
 *
 *       Filename:  client.c
 *    Description:  This file is client main functional
 *                 
 *        Version:  1.0.0(23/12/24)
 *         Author:  dx
 *      ChangeLog:  1, Release initial version on "23/12/24 22:10:04"
 *                 
 ********************************************************************************/
#include <sys/types.h>
#include <sys/socket.h>
#include <stdio.h>
#include <errno.h>
#include <string.h>
#include <unistd.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <stdlib.h>
#include <getopt.h>
#include "ds18b20.h"

#define MSG_STR "hello lingyun apue study!\n"

void print_usage(char *progname)
{
    printf("%s usage:\n", progname);
    printf("-i(--ipaddr): sepecify server IP address\n");
    printf("-p(--port): specify server port.\n");
    printf("-h(--help): print this help information.\n");

    return;
}

int main(int argc, char **argv)
{   
    int cnt =10;
    int sockfd = -1;
    int rv = -1;
    struct sockaddr_in servaddr;
    char *servip = NULL;
    int port = 0;
    char buf[1024];
    struct option opts[] = {
        {"ipaddr", required_argument, NULL, 'i'},
        {"port", required_argument, NULL, 'p'},
        {"help", no_argument, NULL, 'h'},
        {NULL, 0, NULL, 0}};
    int ch;
    int index;
    float  temp=0;

    while ((ch = getopt_long(argc, argv, "i:p:h", opts, NULL)) != -1)
    {
        switch (ch)
        {
        case 'i':
            servip = optarg;
            break;
        case 'p':
            port = atoi(optarg);
            break;
        case 'h':
            print_usage(argv[0]);
            return 0;
        }
    }
    if (!servip || !port)
    {
        print_usage(argv[0]);
        return 0;
    }
    sockfd = socket(AF_INET, SOCK_STREAM, 0);
    if (sockfd < 0)
    {
        printf("Create socket failure : %s\n", strerror(errno));
        return -1;
    }
    printf("create socket[%d]  successfully!\n", sockfd);

    memset(&servaddr, 0, sizeof(servaddr));
    servaddr.sin_family = AF_INET;
    servaddr.sin_port = htons(port);
    inet_aton(servip, &servaddr.sin_addr);
    rv = connect(sockfd, (struct sockaddr *)&servaddr, sizeof(servaddr));
    if (rv < 0)
    {
        printf("connect to server [%s:%d] failure:%s\n", servip, port, strerror(errno));
        return -2;
    }
    printf("connect to server [%s:%d] successful\n", servip, port);
    while (1)
    {   
        temperature(&temp);
        rv = write(sockfd, &temp, sizeof(temp));
        sleep(3);
        
        if (rv < 0)
        {
            printf("wirte to server by sockfd[%d] failure: %s\n", sockfd, strerror(errno));
            return -3;
        }

        memset(buf, 0, sizeof(buf));
        rv = read(sockfd, buf, sizeof(buf));
        if (rv < 0)
        {
            printf("Read data from server by sockfd[%d] failure: %s\n", sockfd, strerror(errno));
            return -4;
        }
        else if (rv == 0)
        {
            printf("socket[%d] get disconnected\n", sockfd);
            return -5;
        }
        else if (rv > 0)
        {
            printf("Read %d bytes data from server: %s\n", rv, buf);
        }
    }
    close(sockfd);

    return 0;
}

