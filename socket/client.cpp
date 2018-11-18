#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <netinet/in.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netdb.h>
#include <unistd.h>

#define MAX_DATA_SIZE 4096
#define SERVER_PORT 12138


int main(int argc,char *argv[])
{
    int sockfd;
    struct hostent * host;
    struct sockaddr_in servAddr;
    int pid;
    char sendBuf[MAX_DATA_SIZE],recvBuf[MAX_DATA_SIZE];
    int sendSize,recvSize;

    host=gethostbyname(argv[1]);
    if(host==NULL)
    {
        perror("get host error");
        exit(-1);
    }

    sockfd=socket(AF_INET,SOCK_STREAM,0);
    if(sockfd==-1)
    {
        perror("创建socket失败");
        exit(-1);
    }

    servAddr.sin_family=AF_INET;
    servAddr.sin_port=htons(SERVER_PORT);
    servAddr.sin_addr=*((struct in_addr *)host->h_addr);
    bzero(&(servAddr.sin_zero),8);

    if(connect(sockfd,(struct sockaddr *)&servAddr,sizeof(struct sockaddr_in))==-1)
    {
        perror("connect 失败");
        exit(-1);
    }

    if((pid=fork())<0)
    {
        perror("fork error");
    }
    else if(pid>0)
    {
        while(1)
        {
            fgets(sendBuf,MAX_DATA_SIZE,stdin);
            sendSize=send(sockfd,sendBuf,MAX_DATA_SIZE,0);
            if(sendSize<0)
                perror("send error");
            memset(sendBuf,0,sizeof(sendBuf));
        }
    }
    else
    {
        while(1)
        {
            recvSize=recv(sockfd,recvBuf,MAX_DATA_SIZE,0);
            if(recvSize<0)
                perror("recv error");
            printf("接收到的信息:%s",recvBuf);
            memset(recvBuf,0,sizeof(recvBuf));
        }
    }
    return 0;
}
