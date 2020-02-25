#ifndef LOGFILE_H_
#define LOGFILE_H_
#include <stdio.h>
#include <error.h>
#include <errno.h>

#include "logserver.h"

enum {
	DEBUG_INFO = 1, //打印普通调试信息
	DEBUG_ERROR,    //打印错误信息
};

#define errstr strerror(errno)
extern int printf_udp(const char* pszFormat, ...);
extern int logfile_init();
extern FILE *popen_redef(const char *command, const char *type);

/**打印宏定义，用于调试
 * level 为打印级别 {DEBUG_INFO ,DEBUG_ERROR}
 */
#ifndef _DEBUG_OPT
#define _DEBUG_OPT 0
#endif
#ifndef _DEBUG_LEVER
#define _DEBUG_LEVER 0
#endif

#if _DEBUG_OPT != 0
#define DEBUGPRINT(level, format, arg...) \
		if(level > _DEBUG_LEVER)\
		{\
			if(level == DEBUG_ERROR)\
			{\
				if(_DEBUG_OPT == 1 || _DEBUG_OPT == 3) {\
					printf(format, ##arg);\
				}\
				if(_DEBUG_OPT == 2 || _DEBUG_OPT == 3) {\
					printf_udp(format, ##arg);\
				}\
			}\
			else {\
				if(_DEBUG_OPT == 1 || _DEBUG_OPT == 3) {\
					printf(format, ##arg);\
				}\
				if(_DEBUG_OPT == 2 || _DEBUG_OPT == 3) {\
					printf_udp(format, ##arg);\
				}\
			}\
		}
#else
#define DEBUGPRINT(level, format, arg...)
#endif

#if _POPEN_REDEF == 1
#define popen(cmd, type)\
		popen_redef(cmd, type)
#endif

#ifdef _DEBUG_TEST
#define LOG_TEST(format, arg...)\
		printf_udp(format, ##arg);
#else
#define LOG_TEST(format, arg...)
#endif

#endif
