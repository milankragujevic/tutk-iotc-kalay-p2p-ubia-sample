#ifndef THREAD_H_
#define THREAD_H_

#ifdef _ITHREAD
typedef struct iThread {
	void *(*run)(void *arg);
	void *arg;
}iThread;
#endif

#define THREAD_MODULE
/*线程基本状态*/
enum {
	PROCESS_RUNNING,
	PROCESS_STOP,
	THREAD_RUNNING,
	THREAD_STOP,
	MSG_HANDLER_ON,
	MSG_HANDLER_OFF,
	NEWS_HANDLER_ON,
	NEWS_HANDLER_OFF,
	ADAPTER_ON,
	ADAPTER_OFF,
};

/*开启线程*/
extern int thread_start(void *(run)(void *));
extern int ithread_start(void *(run)(void *), void *arg);

/*线程同步*/
extern void thread_synchronization(volatile int *thread_r, volatile int *process_r);

/*线程睡眠, ms*/
extern void thread_usleep(int micro_seconds);
extern void host_thread_synchronization(volatile int *r_thread, volatile int *r_host);
#endif
