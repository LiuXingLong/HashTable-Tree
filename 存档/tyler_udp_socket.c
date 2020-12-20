#include <stdio.h>
#include <sys/types.h>
#include <unistd.h>
#include <stdlib.h>
#include <sys/wait.h>
#include <errno.h>
#include <sys/socket.h>
#include <netdb.h>
#include <fcntl.h>
#include <sys/epoll.h>
#include <string.h>
#include <sys/mman.h>
#define MAX_PORT 8
#define MAX_PROCESS 8
#define MAX_CON 1024
#define MAX_EPOLLSIZE 64
#define PORT 9980

// 定义socket队列结构体
typedef struct SOCKET_QUEUE {
	int socket_fd;
	struct SOCKET_QUEUE *prev;
	struct SOCKET_QUEUE *next;
} socket_queue;

// 共享内存数据
typedef struct {
  int         work_n;                      // 当前创建子进程数
  int         connection_n;                // 总处理连接数
  int         work_free_con[MAX_PROCESS];  // 子进程当前空闲连接数
  int         lock;                        // 原子锁 0未锁 1锁了
  int         lock_user;                   // 获取锁用户
  int         pid[MAX_PROCESS];            // 进程pid
} shm_t;

// 原子加
int inc(int *value, int add)
{
    //等同于：value += add
    int old;
    __asm__ volatile ("lock; xaddl %2, %1;" : "=a" (old) : "m" (*value), "a" (add) : "cc","memory");
    return old;
}

// 原子比较交换 
static int cas(int *value,int old,int new)
{
    //等同于： if(value == old) {value = new } else{ ret = value}
    int ret = old;
    __asm__ volatile ("lock; cmpxchg %2, %3" : "=a" (ret), "=m"(*value) : "r" (new), "m" (*value), "0" (old) : "cc","memory");
    return ret == old;
}

// 获取原子锁：获取到返回 true, 否则返回 false
static int shmtx_trylock(int *lock)
{
    return (*lock == 0 && cas(lock, 0, 1));
}

// 设置socket为非阻塞的
static int setNonblockingSocket (int sfd){
  int flags;
  flags = fcntl( sfd, F_GETFL, 0 );
  if (flags == -1) {
    perror( "setNonblockingSocket fcntl" );
    return -1;
  }
  flags |= O_NONBLOCK;
  if (fcntl( sfd, F_SETFL, flags ) == -1){
    perror( "setNonblockingSocket fcntl" );
    return -1;
  }
  return 0;
}

// 设置重复使用地址和端口closesocket后不经历TIME_WAIT的过程
static int setReUseAddr(int fd){
	int reuse = 1;
	//int time_wait = 0;
    setsockopt(fd, SOL_SOCKET, SO_REUSEADDR, (char*)&reuse, sizeof(reuse));
    setsockopt(fd, SOL_SOCKET, SO_REUSEPORT, (char*)&reuse, sizeof(reuse));
    //setsockopt(fd, SOL_SOCKET, SO_DONTLINGER,(char*)&time_wait,sizeof(time_wait));
    //int nNetTimeout=1000;
    //setsockopt(fd, SOL_S0CKET, SO_SNDTIMEO,  (char*)&reuse, sizeof(reuse));
    //setsockopt(fd, SOL_S0CKET, SO_RCVTIMEO,  (char*)&reuse, sizeof(reuse));
	return 0;
}

// 检查是否listenfd
static int checkIsListenfd(int fd, int *fds){
	int i = 0;
	for (i = 0;i < MAX_PORT;i ++) {
		if (fd == *(fds+i)) return *(fds+i);
	}
	return 0;
}

// 创建socket,返回非阻塞的listenfd
static int createSocket(char *port){
	int s, sfd;
	struct addrinfo hints;
  	struct addrinfo *result, *rp;
    memset(&hints, 0, sizeof(struct addrinfo));
    hints.ai_family   = AF_UNSPEC;
    hints.ai_socktype = SOCK_DGRAM; // TCP: SOCK_STREAM  UDP: SOCK_DGRAM
    hints.ai_flags    = AI_PASSIVE;
  	if(s = getaddrinfo(NULL, port, &hints, &result) != 0){
    	printf(stderr, "getaddrinfo: %s\n", gai_strerror(s));
    	return -1;
  	}
  	for (rp = result; rp != NULL; rp = rp->ai_next) {
	    sfd = socket(rp->ai_family, rp->ai_socktype, rp->ai_protocol);
	    if(sfd == -1){
	    	continue;
	    } 
	    if(bind(sfd, rp->ai_addr, rp->ai_addrlen) == 0){
	    	break;
	    }
	    close(sfd);
	}
	if(rp == NULL) {
		fprintf(stderr, "Could not bind\n" );
		return -1;
	}
	freeaddrinfo(result);
	if(setNonblockingSocket(sfd) == -1){
		perror( "setNonblockingSocket error");
	   	abort();
	}
	if (listen(sfd, SOMAXCONN) == -1) {
		perror( "listen error");
		abort();
	}
	setReUseAddr(sfd);
	return sfd;
}

int main()
{
	pid_t pid;
    char s_port[4];
    shm_t *share_data;
	struct epoll_event event;
	struct epoll_event *events;
	int listenfds[MAX_PORT] = {0};
	int i,listenfd,share_lock,epoll_fd,epoll_fds[MAX_PROCESS];

	// 批量创建socket
	for(i = 0; i < MAX_PORT; i++){
		sprintf(s_port,"%d", PORT+i);
		listenfd = createSocket(s_port);
		fprintf("start listen port %d\n", PORT+i);
		if(listenfd == -1){
			perror( "createSocket error");
	      	abort();
		}
		listenfds[i] = listenfd;
	}

	// 添加listenfds
	for(i = 0; i < MAX_PORT; i++){
		event.data.fd = listenfds[i];
  		event.events  = EPOLLIN | EPOLLET;  //读入,边缘触发方式
	}

	// 进程共享内存,原子锁
	share_data = (shm_t *) mmap(NULL, sizeof(shm_t), PROT_READ|PROT_WRITE, MAP_ANON|MAP_SHARED, -1, 0);
	share_data->lock = 0;
	share_data->work_n = 0;
	share_data->connection_n = 0;
	share_data->lock_user = -1;
	for(i = 0; i < MAX_PROCESS; i++){
		share_data->work_free_con[i] = MAX_CON;
		share_data->pid[i] = -1;
	}

	// 创建子进程监听处理事件
	i = 0;
	while(1){
		if(i < MAX_PROCESS){
			i++;
			pid = fork();
		}
		if(pid == 0) {
			int index = share_data->work_n++;
			int n, j, accept_disabled;
			// 创建epoll
			epoll_fd = epoll_create(MAX_EPOLLSIZE);
			if (epoll_fds[i] == -1) {
				perror( "epoll_create" );
				abort();
			}
			for(j = 0; j < MAX_PORT; j++){
				event.data.fd = listenfds[j];
		  		event.events  = EPOLLIN | EPOLLET;  //读入,边缘触发方式
				if (epoll_ctl(epoll_fd, EPOLL_CTL_ADD, listenfds[j], &event) == -1) {
					perror( "epoll_ctl" );
					abort();
				}
			}
			share_data->pid[index] = getpid();
			events = calloc(MAX_EPOLLSIZE, sizeof event);
			printf("子进程pid=%d,进程下标:%d\n", share_data->pid[index],index);
			while(1){
				accept_disabled = 0;
				// 当前子进程无连接数可用
		        if(share_data->work_free_con[index] < MAX_EPOLLSIZE){
		            //是否有办法处理，读写事件
		            accept_disabled = 1;
		        }
		        // 获取到锁修改获取锁用户
		        if(accept_disabled == 0 && shmtx_trylock(&share_data->lock)){
		        	// 处理连接
		          	share_data->lock_user = index;
	        	}else{
	        		continue;
	        	}
	        	// printf("--------进入进程pid=%d--------\n",getpid());
	        	//n = epoll_wait(epoll_fds[index], events, MAX_EPOLLSIZE, -1);
	        	n = epoll_wait(epoll_fd, events, MAX_EPOLLSIZE, 1); //设置过期时间1豪秒很重要
    			for (j = 0; j < n; j++) {
    				printf("--------进入epoll_wait进程pid=%d--------\n",getpid());
    				// 处理异常事件
    				if((events[j].events & EPOLLERR) || (events[j].events & EPOLLHUP) || !(events[j].events & EPOLLIN)){
					   fprintf( stderr, "epoll error\n" );
					   close(events[j].data.fd);
					   share_data->work_free_con[index]++;
					   inc(&share_data->connection_n,-1);
					   //share_data->connection_n--;
					   continue;
					}
					// 处理accept事件
					int listenfd = checkIsListenfd(events[j].data.fd,listenfds);
					if(listenfd){
						while(1){
							struct sockaddr_in client_addr;
							memset(&client_addr, 0, sizeof(struct sockaddr_in));
							socklen_t client_len = sizeof(client_addr);
							int clientfd = accept(listenfd, (struct sockaddr*)&client_addr, &client_len);
							if (clientfd == -1) {
								if ((errno == EAGAIN) || (errno == EWOULDBLOCK)) {
									//已经处理了所有进入的连接
									break;
								} else {
									perror("accept");
									abort();
									break;
								}
				            }
							setNonblockingSocket(clientfd);
							setReUseAddr(clientfd);
							// clientfd添加到epoll
							event.events = EPOLLIN | EPOLLET;
							event.data.fd = clientfd;
							if (epoll_ctl(epoll_fd, EPOLL_CTL_ADD, clientfd, &event) == -1) {
								perror("epoll_ctl");
								abort();
							}
							share_data->work_free_con[index]--;
							inc(&share_data->connection_n,1);
							printf("--------当前建立总连接数:connection_n=%d,空闲连接数:%d,getpid=%d,获取锁进程:pid=%d,锁状态:lock=%d,获取锁进程下标:lock_user=%d,index=%d\n",share_data->connection_n,share_data->work_free_con[index],getpid(),share_data->pid[share_data->lock_user],share_data->lock,share_data->lock_user,index);
							//share_data->connection_n++;
						}
						continue;
					}
					// 处理读事件
					if(events[j].events & EPOLLIN){
			            int done = 0;
			            int connfd = events[j].data.fd;
						while (1) {
							ssize_t buflen;
							char  buf[512];
							buflen = recv(connfd, buf, sizeof(buf), 0);
							if(buflen < 0){
								// 如果errno == EAGAIN，则表示我们已经读取了所有数据,回到主循环。
								if((errno == EAGAIN || errno == EINTR)){
									break;
								}else{
									perror("recv error\n");
						        	done = 1;
								}
								break;
							}else if(buflen == 0){
								// 结束的文件,客户的已关闭连接。
						    	done = 1;
						    	break;
							}
							// 将缓冲区写入标准输出
							if (write(1, buf, buflen) == -1) {
						    	perror( "write error\n");
						    	abort();
							}
						}
						// 关闭描述符将使epoll从被监视的描述符集合中删除它。
						if (done) {
							printf("Closed connection on descriptor %d\n", events[j].data.fd);
							close(events[j].data.fd);
							share_data->work_free_con[index]++;
							inc(&share_data->connection_n,-1);
							//share_data->connection_n--;
						}
						continue;
			       }
    			}
    			// 释放锁
    			share_data->lock = 0;
				share_data->lock_user = -1;
			}
		}
		//printf("当前进程数:%d,当前建立总连接数:connection_n=%d,锁状态:lock=%d,获取锁进程下标:lock_user=%d,获取锁进程:pid=%d\n",share_data->work_n,share_data->connection_n,share_data->lock,share_data->lock_user,share_data->pid[share_data->lock_user]);
		usleep(1000);
	}
	return 0;
}