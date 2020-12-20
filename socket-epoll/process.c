#include <stdio.h>
#include <sys/types.h>
#include <unistd.h>
#include <stdlib.h>
#include <sys/wait.h>
#define COUNT 8

int main()
{
	pid_t pid;
	int cnt = 0;
	int p_cnt = 0;
	while(1){
		if(p_cnt < COUNT){
			p_cnt++;
			pid = fork();
		}
		if(pid == 0) {
			while(1){
				printf("这是子进程,pid=%d，这是第%d次\n",getpid(),cnt);
				cnt++;
				sleep(1);
			}
			exit(0);
		}
		printf("这是父进程,pid=%d\n",getpid());
		sleep(1);
	}
	return 0;
}

/*

#include <stdio.h>
#include <sys/types.h>
#include <unistd.h>
#include <stdlib.h>
#include <sys/wait.h>

int main()
{
	pid_t pid;
	int i, cnt = 0;
	int status;
	pid = fork();
	if(pid < 0){
	 	printf("进程异常");
	 	exit(0);
	}else if(pid > 0){
		wait(&status);
		printf("子进程退出，status:%d\n",WEXITSTATUS(status));
		while(1){
			printf("这是父进程,pid=%d\n",getpid());
			printf("cnt=%d\n",cnt);
			sleep(2);//防止刷屏
		}	
	}else if(pid == 0) {
		for(i = 0;i < 5; i++){//看看是不是保证子进程先运行，五次过后推出进入父进程
			printf("这是子进程,pid=%d，这是第%d次\n",getpid(),i+1);
			cnt++;
			sleep(1);//防止刷屏
		}
		exit(3);
	}
	return 0;
}

*/


