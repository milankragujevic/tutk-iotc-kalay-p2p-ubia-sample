/**	
   @file thread.c
   @brief 线程公用函数
   @note
   author: yangguozheng
   date: 2013-09-16
   mcdify record: creat this file.
   author: yangguozheng
   date: 2013-09-17
   modify record: add a function
 */
#include "common/thread.h"
#include <pthread.h>
#include <string.h>
#include <unistd.h>
#include <stdio.h>
#include <stdlib.h>

/**
   @brief 创建线程
   @author yangguozheng 
   @param[in] run 线程入口
   @return 失败: -1，成功: 线程标识
   @note
 */
#if 1
int thread_start(void *(run)(void *)) {
	int result = ithread_start(run, NULL);
	return result;
}
#endif

/**
   @brief 创建线程底层新封装
   @author yangguozheng 
   @param[in] run 线程入口
   @param[in] arg 线程参数
   @return 失败: -1，成功: 线程标识
   @note
 */
int ithread_start(void *(run)(void *), void *arg) {
	pthread_t threadId;
	pthread_attr_t threadAttr;
	memset(&threadAttr,0,sizeof(pthread_attr_t));
	pthread_attr_init(&threadAttr);
	pthread_attr_setdetachstate(&threadAttr,PTHREAD_CREATE_DETACHED);
	int result = pthread_create(&threadId, &threadAttr, run, arg);
	pthread_attr_destroy(&threadAttr);
	printf("%d\n", threadId);
	return threadId;
}

/**
   @brief 线程同步
   @author yangguozheng 
   @param[in] thread_r 当前线程同步变量
   @param[in] process_r 要等待的同步变量
   @return 
   @note
     阻塞
 */
void thread_synchronization(volatile int *thread_r, volatile int *process_r) {
	*thread_r = 1;
	while(*process_r == 0) {
		thread_usleep(0);
	}
	thread_usleep(0);
}

/**
   @brief 线程睡眠
   @author yangguozheng 
   @param[in] micro_seconds 睡眠时间，毫秒
   @return 
   @note
 */
void thread_usleep(int micro_seconds) {
	usleep(micro_seconds);
}

/**
   @brief host_thread_synchronization
   @author  
   @param[in] r_thread 
   @param[in] r_host 
   @return 
   @note
 */
void host_thread_synchronization(volatile int *r_thread, volatile int *r_host){
	while (1) {
		if (*r_thread == 1) {
//			printf("rHost%d\n",*r_host);
			*r_host = 1;
			break;
		}
		thread_usleep(0);
	}
	thread_usleep(0);
}
