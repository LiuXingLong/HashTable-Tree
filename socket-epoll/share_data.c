/** 惊群问题：多进程共享内容原子锁 **/
#include <stdio.h>
#include <sys/types.h>
#include <unistd.h>
#include <stdlib.h>
#include <sys/wait.h>
#include <sys/mman.h>
#include <pthread.h>
#define COUNT 4
#define MAXCON 1024
#define MAXTHREADS 10
pthread_mutex_t mutex = PTHREAD_MUTEX_INITIALIZER;

// 共享内存数据
typedef struct {
  int         work_n;                // 当前创建子进程数
  int         connection_n;          // 总处理连接数
  int         work_free_con[COUNT];  // 子进程当前空闲连接数
  int         lock;
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
int cas(int *value,int old,int new)
{
    //等同于： if(value == old) {value = new } else{ ret = value}
    int ret = old;
    __asm__ volatile ("lock; cmpxchg %2, %3" : "=a" (ret), "=m"(*value) : "r" (new), "m" (*value), "0" (old) : "cc","memory");
    return ret == old;
}

// 获取原子锁：获取到返回 true, 否则返回 false
int shmtx_trylock(int *lock)
{
    return (*lock == 0 && cas(lock, 0, 1));
}

/*
// 申请共享内存
void shm_alloc(shm_t *shm)
{
    // MAP_ANON:不使用文件映射方式，因此fd,offset无用,相当于在内存开辟一块空间用于共享,由master创建
    shm->addr = (u_char *) mmap(NULL, shm->size, PROT_READ|PROT_WRITE, MAP_ANON|MAP_SHARED, -1, 0);
    if (shm->addr == MAP_FAILED) {
        printf("mmap(MAP_ANON|MAP_SHARED, %uz) failed\n", shm->size);
    }
}

// 释放共享内存
void shm_free(shm_t *shm)
{
    if (munmap((void *) shm->addr, shm->size) == -1) {
        printf("munmap(%p, %uz) failed\n", shm->addr, shm->size);
    }
}
*/

int main()
{
  int i;
  pid_t pid;
  int p_cnt = 0;
  shm_t *share_data;
  share_data = (shm_t *) mmap(NULL, sizeof(shm_t), PROT_READ|PROT_WRITE, MAP_ANON|MAP_SHARED, -1, 0);
  share_data->lock = 0;
  share_data->work_n = 0;
  share_data->connection_n = 0;
  for(i = 0; i < COUNT; i++){
    share_data->work_free_con[i] = MAXCON;
  }
  while(1){
    if(p_cnt < COUNT){
      p_cnt++;
      pid = fork();
    }
    if(pid == 0){
      int index = share_data->work_n++;
      printf("这是子进程,pid=%d，这是第%d个进程\n",getpid(),index);
      while(1){
        // 当前子进程无连接数可用
        if(share_data->work_free_con[index] <= 0){
          //是否有办法处理，读写事件
          continue;
        }
        // 未获取到锁直接返回
        if(!shmtx_trylock(&share_data->lock)){
          continue;
        }
        /*
        n = epoll_wait();
        for (i = 0; i < n; i++) {
          if(sfd == events[i].data.fd){
            while (1) {
              accept();
            }
            continue;
          }
          if(events[i].events & EPOLLIN){
            while (1) {
              read()
            }
            continue;
          }
          if(events[i].events & EPOLLOUT){
            write()
            continue;
          }
          if((events[i].events & EPOLLERR) || 
            (events[i].events & EPOLLHUP) || 
            !((events[i].events & EPOLLIN) || (events[i].events & EPOLLOUT))){
            //此fd上发生错误，或套接字无法读取
            close();
            continue;
          }
        }
        */
        // 处理连接 accept
        share_data->connection_n++;
        share_data->work_free_con[index]--;
        printf("【建立连接】这是子进程,pid=%d，当前空闲连接数%d\n",getpid(),share_data->work_free_con[index]);
        usleep(50000);
        //实际情况:当连接关闭时加1 close
        //share_data->work_free_con[index]++;
        //epoll_wait 处理完释放锁
        share_data->lock = 0;
      }
      exit(0);
    }
    printf("这是父进程,pid=%d,当前总建立连接数connection_n=%d\n",getpid(),share_data->connection_n);
    sleep(1);
  }
  munmap(share_data, sizeof(shm_t));
  return 0;
}


