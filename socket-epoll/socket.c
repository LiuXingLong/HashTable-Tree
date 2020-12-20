#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <errno.h>
#include <sys/socket.h>
#include <netdb.h>
#include <fcntl.h>
#include <sys/epoll.h>
#include <string.h>
#define MAXEVENTS 64

/*
 * 函数:
 * 功能:创建和绑定一个TCP socket
 * 参数:端口
 * 返回值:创建的socket
 */
static int create_and_bind( char *port ) {
  struct addrinfo hints;
  struct addrinfo *result, *rp;
  int   s, sfd;
  memset( &hints, 0, sizeof(struct addrinfo) );
  hints.ai_family   = AF_UNSPEC;
  /* Return IPv4 and IPv6 choices */
  hints.ai_socktype = SOCK_STREAM;
  /* We want a TCP socket */
  hints.ai_flags    = AI_PASSIVE;
  /* All interfaces */
  s = getaddrinfo( NULL, port, &hints, &result );
  if ( s != 0 ) {
    printf( stderr, "getaddrinfo: %s\n", gai_strerror( s ) );
    return(-1);
  }
  for ( rp = result; rp != NULL; rp = rp->ai_next ) {
    sfd = socket( rp->ai_family, rp->ai_socktype, rp->ai_protocol );
    if ( sfd == -1 )
          continue;
    s = bind( sfd, rp->ai_addr, rp->ai_addrlen );
    if ( s == 0 ) {
      /* We managed to bind successfully! */
      break;
    }
    close( sfd );
  }
  if ( rp == NULL ) {
    fprintf( stderr, "Could not bind\n" );
    return(-1);
  }
  freeaddrinfo( result );
  return(sfd);
}

/*
 * 函数
 * 功能:设置socket为非阻塞的
 */
static int make_socket_non_blocking( int sfd ) {
  int flags, s;
  /* 得到文件状态标志 */
  flags = fcntl( sfd, F_GETFL, 0 );
  if ( flags == -1 ) {
    perror( "fcntl" );
    return(-1);
  }
  /* 设置文件状态标志 */
  flags |= O_NONBLOCK;
  s = fcntl( sfd, F_SETFL, flags );
  if ( s == -1 ) {
    perror( "fcntl" );
    return(-1);
  }
  return(0);
}

// 端口由参数argv[1]指定
int main( int argc, char *argv[] ) {
  int     sfd, s;
  int     efd;
  struct epoll_event  event;
  struct epoll_event  *events;
  if (argc != 2) {
    fprintf( stderr, "Usage: %s [port]\n", argv[0] );
    exit( EXIT_FAILURE );
  }
  sfd = create_and_bind( argv[1] );
  if ( sfd == -1 )
      abort();
  s = make_socket_non_blocking( sfd );
  if ( s == -1 )
      abort();
  s = listen( sfd, SOMAXCONN );
  if ( s == -1 ) {
    perror( "listen" );
    abort();
  }
  // 除了参数size被忽略外,此函数和epoll_create完全相同
  efd = epoll_create1(0);
  if ( efd == -1 ) {
    perror( "epoll_create" );
    abort();
  }
  event.data.fd = sfd;
  event.events  = EPOLLIN | EPOLLET;
  // 读入,边缘触发方式
  s   = epoll_ctl( efd, EPOLL_CTL_ADD, sfd, &event );
  if ( s == -1 ) {
    perror( "epoll_ctl" );
    abort();
  }
  // Buffer where events are returned 返回事件的缓冲区
  events = calloc( MAXEVENTS, sizeof event );
  // The event loop 事件循环
  while (1) {
    int n, i;
    n = epoll_wait(efd, events, MAXEVENTS, -1 );
    for (i = 0; i < n; i++) {
       if((events[i].events & EPOLLERR) || (events[i].events & EPOLLHUP) || !((events[i].events & EPOLLIN) || (events[i].events & EPOLLOUT))){
          //此fd上发生错误，或套接字无法读取
          fprintf( stderr, "epoll error\n" );
          close(events[i].data.fd);
          continue;
       }
       if(sfd == events[i].data.fd){
          // 我们在监听套接字上有一个通知，这意味着一个或多个客户端接入连接
          while (1) {
            struct sockaddr in_addr;
            socklen_t in_len;
            int   infd;
            char    hbuf[NI_MAXHOST], sbuf[NI_MAXSERV];
            in_len  = sizeof in_addr;
            infd = accept( sfd, &in_addr, &in_len );
            if (infd == -1) {
              if ( (errno == EAGAIN) || (errno == EWOULDBLOCK) ) {
                // 我们已经处理了所有进入的连接
                break;
              } else {
                perror( "accept" );
                break;
              }
            }
            // 将地址转化为主机名或者服务名
            s = getnameinfo( &in_addr, in_len,hbuf, sizeof hbuf,sbuf, sizeof sbuf, NI_NUMERICHOST | NI_NUMERICSERV );
            // flag参数:以数字名返回
            // 主机地址和服务地址
            if ( s == 0 ) {
              printf( "Accepted connection on descriptor %d " "(host=%s, port=%s)\n", infd, hbuf, sbuf );
            }
            // 使传入套接字是非阻塞的，并将其添加到要监视的fds列表中 
            s = make_socket_non_blocking( infd );
            if ( s == -1 ) abort();
            event.data.fd = infd;
            event.events  = EPOLLIN | EPOLLET;
            s   = epoll_ctl( efd, EPOLL_CTL_ADD, infd, &event );
            if ( s == -1 ) {
              perror( "epoll_ctl" );
              abort();
            }
          }
          continue;
      }
      if(events[i].events & EPOLLIN){
          int done = 0;
          int connfd = events[i].data.fd;
          while (1) {
            ssize_t count;
            char  buf[512];
            count = read(connfd, buf, sizeof(buf));
            if (count == -1) {
              // 如果errno == EAGAIN，则表示我们已经读取了所有数据。回到主循环。
              if ( errno != EAGAIN ) {
                perror( "read" );
                done = 1;
              }
              break;
            } else if ( count == 0 ) {
              // 结束的文件。客户的已关闭连接。
              done = 1;
              break;
            }
            // 将缓冲区写入标准输出
            s = write( 1, buf, count );
            if (s == -1) {
              perror( "write" );
              abort();
            }
          }
          // 关闭描述符将使epoll从被监视的描述符集合中删除它。
          if (done) {
            printf( "Closed connection on descriptor %d\n", events[i].data.fd );
            close( events[i].data.fd );
          } else {
              event.data.fd = connfd;
              event.events  = EPOLLIN | EPOLLOUT | EPOLLET;
              s = epoll_ctl(efd, EPOLL_CTL_MOD, connfd, &event);
              if (s == -1) {
                perror( "epoll_ctl" );
                abort();
              }
          }
          continue;
       }
       if(events[i].events & EPOLLOUT){
          const char* vptr = "hi client!";
          int connfd = events[i].data.fd;
          size_t count = strlen(vptr);
          s = write(connfd, vptr, count );
          if (s == -1) {
            perror( "write" );
            abort();
          }
          continue;
       }      
    }
  }
  free( events );
  close( sfd );
  return(EXIT_SUCCESS);
}



/*
// 端口由参数argv[1]指定 
int main( int argc, char *argv[] ) {
  int     sfd, s;
  int     efd;
  struct epoll_event  event;
  struct epoll_event  *events;
  if ( argc != 2 ) {
    fprintf( stderr, "Usage: %s [port]\n", argv[0] );
    exit( EXIT_FAILURE );
  }
  sfd = create_and_bind( argv[1] );
  if ( sfd == -1 )
      abort();
  s = make_socket_non_blocking( sfd );
  if ( s == -1 )
      abort();
  s = listen( sfd, SOMAXCONN );
  if ( s == -1 ) {
    perror( "listen" );
    abort();
  }
  // 除了参数size被忽略外,此函数和epoll_create完全相同
  efd = epoll_create1( 0 );
  if ( efd == -1 ) {
    perror( "epoll_create" );
    abort();
  }
  event.data.fd = sfd;
  event.events  = EPOLLIN | EPOLLET;
  // 读入,边缘触发方式
  s   = epoll_ctl( efd, EPOLL_CTL_ADD, sfd, &event );
  if ( s == -1 ) {
    perror( "epoll_ctl" );
    abort();
  }
  // Buffer where events are returned 返回事件的缓冲区
  events = calloc( MAXEVENTS, sizeof event );
  // The event loop 事件循环
  while ( 1 ) {
    int n, i;
    n = epoll_wait( efd, events, MAXEVENTS, -1 );
    for ( i = 0; i < n; i++ ) {
      if ( (events[i].events & EPOLLERR) || (events[i].events & EPOLLHUP) || (!(events[i].events & EPOLLIN) ) ) {
        // An error has occured on this fd, or the socket is not ready for reading (why were we notified then?) 
        // 此fd上发生错误，或套接字无法读取(为什么会通知我们?)
        fprintf( stderr, "epoll error\n" );
        close( events[i].data.fd );
        continue;
      } else if (sfd == events[i].data.fd ) {
        // We have a notification on the listening socket, which means one or more incoming connections. 
        // 我们在监听套接字上有一个通知，这意味着一个或多个传入连接。
        while (1) {
          struct sockaddr in_addr;
          socklen_t in_len;
          int   infd;
          char    hbuf[NI_MAXHOST], sbuf[NI_MAXSERV];
          in_len  = sizeof in_addr;
          infd = accept( sfd, &in_addr, &in_len );
          if (infd == -1) {
            if ( (errno == EAGAIN) || (errno == EWOULDBLOCK) ) {
              // We have processed all incoming connections. 
              // 我们已经处理了所有进入的连接
              break;
            } else {
              perror( "accept" );
              break;
            }
          }
          // 将地址转化为主机名或者服务名
          s = getnameinfo( &in_addr, in_len,hbuf, sizeof hbuf,sbuf, sizeof sbuf, NI_NUMERICHOST | NI_NUMERICSERV );
          // flag参数:以数字名返回
          // 主机地址和服务地址
          if ( s == 0 ) {
            printf( "Accepted connection on descriptor %d " "(host=%s, port=%s)\n", infd, hbuf, sbuf );
          }
          // Make the incoming socket non-blocking and add it to the list of fds to monitor. 
          // 使传入套接字是非阻塞的，并将其添加到要监视的fds列表中 
          s = make_socket_non_blocking( infd );
          if ( s == -1 ) abort();
          event.data.fd = infd;
          event.events  = EPOLLIN | EPOLLET;
          s   = epoll_ctl( efd, EPOLL_CTL_ADD, infd, &event );
          if ( s == -1 ) {
            perror( "epoll_ctl" );
            abort();
          }
        }
        continue;
      } else {
        // We have data on the fd waiting to be read. Read and display it. We must read whatever data is available completely, as we are running in edge-triggered mode and won't get a notification again for the same data. 
        // 我们有fd的数据等待读取。阅读和显示它。我们必须阅读任何可以得到的数据完全，因为我们运行在边缘触发模式并且不会再次收到通知数据。
        int done = 0;
        while (1) {
          ssize_t count;
          char  buf[512];
          count = read(events[i].data.fd, buf, sizeof(buf));
          if ( count == -1 ) {
            // If errno == EAGAIN, that means we have read all data. So go back to the main loop.
            // 如果errno == EAGAIN，则表示我们已经读取了所有数据。回到主循环。
            if ( errno != EAGAIN ) {
              perror( "read" );
              done = 1;
            }
            break;
          } else if ( count == 0 ) {
            // End of file. The remote has closed the connection. 
            // 结束的文件。遥控器已关闭连接。
            done = 1;
            break;
          }
          // Write the buffer to standard output 
          // 将缓冲区写入标准输出
          s = write( 1, buf, count );
          if ( s == -1 ) {
            perror( "write" );
            abort();
          }
        }
        if ( done ) {
          printf( "Closed connection on descriptor %d\n", events[i].data.fd );           
          //Closing the descriptor will make epoll remove it from the set of descriptors which are monitored. 
          //关闭描述符将使epoll从被监视的描述符集合中删除它。
          close( events[i].data.fd );
        }
      }
    }
  }
  free( events );
  close( sfd );
  return(EXIT_SUCCESS);
}
*/



/*
#include <stdio.h>
#include <stdlib.h>
#include <netdb.h>
#include <sys/types.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <fcntl.h>
#include <sys/socket.h>
#include <sys/epoll.h>
#include <iostream>
#include <string.h>
#include <vector>
#include <errno.h>
using namespace std;
int setSocketNonblocking(int fd)
{
    //将监听socker设置为非阻塞的
    int oldSocketFlag = fcntl(fd, F_GETFL, 0);
    int newSocketFlag = oldSocketFlag | O_NONBLOCK;
    if(fcntl(fd, F_SETFL, newSocketFlag)==-1) 
    {
        close(fd);
        cout << "set listenfd to nonblock error" << endl;
        return -1;
    }
    return oldSocketFlag;
}

int main()
{
    //创建一个监听socket
    int listenfd = socket(AF_INET, SOCK_STREAM, 0);
    if(listenfd == -1)
    {
        cout << "create listen socket error" << endl;
        return -1;
    }
    
    setSocketNonblocking(listenfd);

    //初始化服务器地址
    
    struct sockaddr_in bindaddr;
    bindaddr.sin_family = AF_INET;
    bindaddr.sin_addr.s_addr = htonl(INADDR_ANY);
    bindaddr.sin_port = htons(3100);

    if(bind(listenfd, (struct sockaddr*)&bindaddr, sizeof(bindaddr))==-1)
    {
        cout << "bind listen socker error." << endl;
        close(listenfd);
        return -1;
    }
    
    //启动监听
    if(listen(listenfd, SOMAXCONN)==-1)
    {
        cout << "listen error." << endl;
        close(listenfd);
        return -1;
    }
    
    //复用地址和端口号
    int on = 1;
    setsockopt(listenfd, SOL_SOCKET, SO_REUSEADDR, (char*)&on, sizeof(on));
    setsockopt(listenfd, SOL_SOCKET, SO_REUSEPORT, (char*)&on, sizeof(on));


    //创建epollfd
    int epollfd = epoll_create(1);
    if(epollfd == -1)
    {
        cout << "create epollfd error." << endl;
        close(listenfd);
        return -1;
    }
    
    epoll_event listen_fd_event;
    listen_fd_event.data.fd = listenfd;
    listen_fd_event.events = EPOLLIN;
    listen_fd_event.events |= EPOLLET;
    
    //将监听sokcet绑定到epollfd上去
    if(epoll_ctl(epollfd, EPOLL_CTL_ADD, listenfd,&listen_fd_event)==-1)
    {
        cout << "epoll_ctl error" << endl;
        close(listenfd);
        return -1;
    }
    
    int n;
    while(true)
    {
        epoll_event epoll_events[1024];
        n = epoll_wait(epollfd, epoll_events, 1024, 1000);
        if(n<0)
        {
            //被信号中断
            if(errno == EINTR) continue;
            //出错,退出
            break;
        }
        else if(n==0)
        {
            //超时,继续
            continue;
        }
        for(size_t i = 0; i<n;i++)
        {
            //事件可读
            if(epoll_events[i].events & EPOLLIN)
            {
                if(epoll_events[i].data.fd == listenfd)
                {
                    //侦听socket,接受新连接
                    struct sockaddr_in clientaddr;
                    socklen_t clientaddrlen = sizeof(clientaddr);
                    int clientfd = accept(listenfd, (struct sockaddr*)&clientaddr, &clientaddrlen);
                    if(clientfd != -1)
                    {
                        int oldSocketFlag = fcntl(clientfd, F_GETFL,0);
                        int newSocketFlag = oldSocketFlag | O_NONBLOCK;
                        if(fcntl(clientfd, F_SETFD, newSocketFlag)==-1)
                        {
                            close(clientfd);
                            cout << "set clientfd to nonblocking error." << endl;
                        }
                        else 
                        {
                            epoll_event client_fd_event;
                            client_fd_event.data.fd = clientfd;
                            client_fd_event.events = EPOLLIN;
                            client_fd_event.events |= EPOLLET; //设置为边缘出发
                            if(epoll_ctl(epollfd, EPOLL_CTL_ADD, clientfd, &client_fd_event)!=-1)
                            {
                                cout << "new client accepted,clientfd: " << clientfd << endl;
                            }
                            else 
                            {
                                cout << "add client fd to epollfd error" << endl;
                                close(clientfd);
                            }
                        }
                    }
                }
                else
                {
                    //普通clientfd
                    char ch;
                    int m = recv(epoll_events[i].data.fd, &ch, 1, 0);
                    if(m==0)
                    {
                        //对端关闭了连接，从epollfd上移除clientfd
                        if(epoll_ctl(epollfd, EPOLL_CTL_DEL, epoll_events[i].data.fd,NULL)!=-1)
                        {
                            cout << "client disconnected,clientfd:" <<epoll_events[i].data.fd << endl;
                        }
                        close(epoll_events[i].data.fd);
                    }
                    else if(m<0)
                    {
                        //出错
                        if(errno!= EWOULDBLOCK && errno !=EINTR)
                        {
                            if(epoll_ctl(epollfd, EPOLL_CTL_DEL, epoll_events[i].data.fd,NULL)!=-1)
                            {
                                cout << "client disconnected,clientfd:" <<epoll_events[i].data.fd << endl;
                            }
                            close(epoll_events[i].data.fd);
                        }
                        break;
                    }
                    else 
                    {
                        //正常收到数据
                        cout << "recv from client:" << epoll_events[i].data.fd << " " << ch << endl; 
                    }
                }
            }
            else if(epoll_events[i].events & POLLERR)
            {
                        // TODO 暂不处理
            }      
        }
    }
    close(listenfd);
    return 0;
}
*/


