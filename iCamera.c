/**	
   @file iCamera.c
   @brief 应用程序主文件
   @note
   author: yangguozheng
   date: 2013-09-16
   mcdify record: creat this file.
   author: yangguozheng
   date: 2013-09-17
   modify record: add a function
 */
#include <stdio.h>
#include <pthread.h>
#include <unistd.h>
#include <string.h>
#include <stdlib.h>
#include <sys/wait.h>
#include <sys/ioctl.h>
#include <signal.h>
#include <fcntl.h>
#include <error.h>
#include <sys/types.h>   //added 2013.6.3
#include <errno.h>
#include <regex.h>
#include <termios.h>
#include "common/logfile.h"
#include "Udpserver/udpServer.h"

#include "MFI/MFI.h"
//#include "Child_Process/childProcess.h"
#include "Child_Process/child_process.h"
#include "common/msg_queue.h"
#include <math.h>


//#include "Video/video.h"
//extern int world_main();
//extern void hello_thread_start();

/** yangguozheng, 标记全局消息队列id*/
typedef struct iCamera_t {
	int rMsgId; ///< 目标消息队列id
	int lMsgId; ///< 本地消息队列id
}iCamera_t;

/** @brief 管道*/
int fdPipe[2];

/** @brief 进程运行状态*/
int bExit = 0;

/**
   @应用程序入口
   @author <yangguozheng, wangfengbo>
   @return <进程返回信息>
   @note
 */
int main(void) {
//	test_main();
//	return 0;
//	test_msg_queue();
	//world_main();
	//hello_thread_start();
	pid_t pid;

	if(0 > pipe(fdPipe))
	{
		return -1;
	}

	if(0 > (pid = fork()) )
	{
		return -2;
	}
	//父进程，通过读管道数据，检测子进程功能是否正常
	else if(pid > 0)
	{
		//char szWord[2] = {0};

		//关闭管道写端
		close(fdPipe[1]);
//		signal(SIGINT, SIG_DFL);
//保护模式，若_GUARD_MODE为关闭则以正常模式运行，否则以守护模式运行，后者会屏蔽一些系统信号，具体参见NewsUtils.c
#if _GUARD_MODE == 0
		while(!bExit)
		{
			//从管道读取数据
		//	read(fdPipe[0], szWord, 1);

		//	printf("%s\n",szWord);
//			printf("helllo %d\n", getpid());
			//每10秒写一次
			sleep(10);
		}
	//等待子进程退出
		wait(0);
#else
		exit(0);
#endif
	}
#if _GUARD_MODE == 1
	setsid();
#if 0
	if(pid =fork())
	{
		exit(0);
	}
#endif
	chdir("/");
	umask(0);
	/*for(i=0;i<getdtablesize();i++)
	{
		close(i);
	}*/
#endif

	//******************以下为子进程工作内容*******************
	//关闭管道读端
	close(fdPipe[0]);
	//udpServer_threadStart();
//	Create_Alarm_delay_Thread();
//	DEBUGPRINT(DEBUG_INFO,"long int ======%i\n",sizeof(long int));
//	CreateVideoCaptureThread();


//	child_processThread();
	//kill(getppid(), SIGINT);
	//子进程入口,exin
	child_process_main();

	return EXIT_SUCCESS;
}
