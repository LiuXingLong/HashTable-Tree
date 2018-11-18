#include <stdio.h>
#include <stdlib.h>
#include <sys/socket.h>
#include <sys/epoll.h>
#include <string.h>
#include <netinet/in.h>
#include <string.h>
#include <netdb.h>
#include <arpa/inet.h>
#include <unistd.h>

#define SERVER_PORT 12138
#define CON_QUEUE 50
#define MAX_DATA_SIZE 4096
#define MAX_EVENTS 500000

void AcceptConn(int sockfd,int epollfd);
void Handle(int clientfd);

int main(int argc,char *argv[])
{
    struct sockaddr_in serverSockaddr;
    int sockfd;

    //创建socket
    if((sockfd=socket(AF_INET,SOCK_STREAM,0))==-1)
    {
        perror("创建socket失败");
        exit(-1);
    }
    serverSockaddr.sin_family=AF_INET;
    serverSockaddr.sin_port=htons(SERVER_PORT);
    serverSockaddr.sin_addr.s_addr=htonl(INADDR_ANY);
    bzero(&(serverSockaddr.sin_zero),8);

    int on=1;
    setsockopt(sockfd,SOL_SOCKET,SO_REUSEADDR,&on,sizeof(on));

    if(bind(sockfd,(struct sockaddr *)&serverSockaddr,sizeof(struct sockaddr))==-1)
    {
        perror("绑定失败");
        exit(-1);
    }

    if(listen(sockfd,CON_QUEUE)==-1)
    {
        perror("监听失败");
        exit(-1);
    }

    //epoll初始化
    int epollfd;//epoll描述符
    struct epoll_event eventList[MAX_EVENTS];
    epollfd=epoll_create(MAX_EVENTS);
    struct epoll_event event;
    event.events=EPOLLIN|EPOLLET;
    event.data.fd=sockfd;//把server socket fd封装进events里面

    //epoll_ctl设置属性,注册事件
    if(epoll_ctl(epollfd,EPOLL_CTL_ADD,sockfd,&event)<0)
    {
        printf("epoll 加入失败 fd:%d\n",sockfd);
        exit(-1);
    }

    while(1)
    {   
        int timeout=1;//设置超时;在select中使用的是timeval结构体
        //epoll_wait epoll处理
        //ret会返回在规定的时间内获取到IO数据的个数，并把获取到的event保存在eventList中，注意在每次执行该函数时eventList都会清空，由epoll_wait函数填写。
        //而不清除已经EPOLL_CTL_ADD到epollfd描述符的其他加入的文件描述符。这一点与select不同，select每次都要进行FD_SET，具体可看我的select讲解。
        //epoll里面的文件描述符要手动通过EPOLL_CTL_DEL进行删除。
        int ret=epoll_wait(epollfd,eventList,MAX_EVENTS,timeout);

        if(ret<0)
        {
             printf("epoll error 1\n");
            //perror("epoll error\n");
            //break;
            continue;
        }
        else if(ret==0)
        {
            //超时
            //printf("超时\n");
            continue;
        }

        //直接获取了事件数量，给出了活动的流，这里就是跟selec，poll区别的关键 //select要用遍历整个数组才知道是那个文件描述符有事件。而epoll直接就把有事件的文件描述符按顺序保存在eventList中
        for(int i=0;i<ret;i++)
        {
            //错误输出
            if((eventList[i].events & EPOLLERR) || (eventList[i].events & EPOLLHUP) || !(eventList[i].events & EPOLLIN))
            {
                printf("epoll error 2\n");
                close(eventList[i].data.fd);
                continue;
                //exit(-1);
            }

            if(eventList[i].data.fd==sockfd)
            {
                //这个是判断sockfd的，主要是用于接收客户端的连接accept
                AcceptConn(sockfd,epollfd);
            }
            else //里面可以通过判断eventList[i].events&EPOLLIN 或者 eventList[i].events&EPOLLOUT 来区分当前描述符的连接是对应recv还是send
            {
                //其他所有与客户端连接的clientfd文件描述符
                //获取数据等操作
                //如需不接收客户端发来的数据，但是不关闭连接。
                //epoll_ctl(epollfd, EPOLL_CTL_DEL,eventList[i].data.fd,eventList[i]);
                //Handle对各个客户端发送的数据进行处理
                Handle(eventList[i].data.fd);
                epoll_ctl(epollfd, EPOLL_CTL_DEL,eventList[i].data.fd,&eventList[i]);
                close(eventList[i].data.fd);
            }
        }
    }

    close(epollfd);
    close(sockfd);
    return 0;
}

void AcceptConn(int sockfd,int epollfd)
{
    struct sockaddr_in sin;
    socklen_t len=sizeof(struct sockaddr_in);
    bzero(&sin,len);

    int confd=accept(sockfd,(struct sockaddr *)&sin,&len);

    if(confd<0)
    {
        perror("connect error\n");
        exit(-1);
    }

    //把客户端新建立的连接添加到EPOLL的监听中
    struct epoll_event event;
    event.data.fd=confd;
    event.events=EPOLLIN|EPOLLET;
    epoll_ctl(epollfd,EPOLL_CTL_ADD,confd,&event);
    return ;
}

void Handle(int clientfd)
{
    int recvLen=0;
    int sendLen=0;
    char recvBuf[MAX_DATA_SIZE];
    char sendBuf[MAX_DATA_SIZE];
    memset(recvBuf,0,sizeof(recvBuf));
    sprintf(sendBuf, "HTTP/1.1 200 OK\nServer: nginx/1.11.10\nDate: Wed, 08 Aug 2018 07:49:23 GMT\nContent-Type: text/html; charset=UTF-8\n\nSuccess");
    recvLen=recv(clientfd,(char *)recvBuf,MAX_DATA_SIZE,0);
    if(recvLen==0)
        return ;
    else if(recvLen<0)
    {
        perror("recv Error");
        exit(-1);
    }
    //各种处理
    sendLen=send(clientfd,sendBuf,strlen(sendBuf),0);
    if(sendLen==0){
       return ;
    }else if(sendLen<0){
       perror("send Error");
       exit(-1);
    }
   // printf("接收到的数据:\n%s",recvBuf);
   // printf("发送出的数据:\n%s",sendBuf);
    return ;
}
