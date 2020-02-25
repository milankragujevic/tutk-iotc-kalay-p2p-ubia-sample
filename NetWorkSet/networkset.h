#ifndef NETWORKSET_H_
#define NETWORKSET_H_


/* 同子进程通信协议 */
enum
{
	MSG_NETWORKSET_T_SET_NET = 1,
	MSG_NETWORKSET_P_SET_NET_TYPE,                    //设置网络类型
	MSG_NETWORKSET_P_SET_IP_TYPE,                     //设置ip类型
	MSG_NETWORKSET_P_REPORT_CONNECT_RESULT,           //网络设置结果
	MSG_NETWORKSET_T_CLEAN_RESOURCE,                  //清理有线资源 当网线
	MSG_NETWORKSET_P_START_SET_NET,                   //清理有线资源 当网线
	MSG_NETWORKSET_T_NET_CONNECT_STATUS,              //网络连接结果  0：成功  1：失败
	MSG_NETWORKSET_P_WIFI_LOGIN_STATUS,               //WIFI结果     0：成功  1：失败
	MSG_NETWORKSET_P_WIFI_DISCONNECT,                 //WIFI断连处理
	MSG_NETWORKSER_T_NET_START_PING,                  //tutk通知网络线程开始ping
};


void test_net_set();

/**
  *  @brief      网络设置线程创建函数
  *  @author     <wfb> 
  *  @param[in]  <无>
  *  @param[out] <无>
  *  @return     <无>
  *  @remark     2013-09-03 增加该注释
  *  @note
*/
int NetworkSet_Thread_Creat(void);

#endif /* THREAD_H_ */
