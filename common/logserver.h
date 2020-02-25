/*
 * logserver.h
 *
 *  Created on: Jan 26, 2014
 *      Author: root
 */

#ifndef LOGSERVER_H_
#define LOGSERVER_H_

/**
 @brief 子进程消息打印
 @author yangguozheng
 @param[in] type 消息类型
 @param[in] cmd  消息命令
 @return
 @note
 */
extern void log_m(char type, char cmd);

/**
 @brief tcp消息打印
 @author yangguozheng
 @param[in] data 数据
 @param[in] len  数据长度
 @return
 @note
 */
extern void log_b(void *data, int len);

/**
 @brief https消息打印
 @author yangguozheng
 @param[in] data 数据
 @param[in] len  数据长度
 @return
 @note
 */
extern void log_s(void *data, int len);

/**
 @brief 日志参数配置
 @author yangguozheng
 @param[in] cMaxLen           子进程最大消息条数
 @param[in] bMaxSize          二进制信息每条大小
 @param[in] bMaxLen           二进制信息最大条数
 @param[in] bStartFlag        二进制消息开头
 @param[in] strBufMaxSize     字符缓冲区大小
 @param[in] strStartFlag      字符消息开始标识
 @param[in] fileStartFlag     日志文件开始标识
 @param[in] httpsServerAddr   https服务器地址
 @param[in] httpsServerPort   https服务器端口
 @param[in] httpsServerUrl    https服务接口
 @param[in] tcpPort           tcp监听端口
 @param[in] timeout           超时时间
 @return
 @note
 */
extern int logServerConf(int cMaxLen, int bMaxSize, int bMaxLen, const char *bStartFlag,
		int strBufMaxSize,
		const char *strStartFlag, const char *fileStartFlag, const char *httpsServerAddr,
		int httpsServerPort, const char *httpsServerUrl, int tcpPort, int timeout);

/**
 @brief 开启服务
 @author yangguozheng
 @param[in] opt 可选参数，当为1时已默认值开启
 @return 0
 @note
 */
extern int log_start(int opt);

extern char *log_getVersion();

extern int log_printf(const char* format, ...);


#endif /* LOGSERVER_H_ */
