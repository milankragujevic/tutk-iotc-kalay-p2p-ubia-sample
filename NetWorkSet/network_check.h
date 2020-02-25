#ifndef NETWORK_CHECK_H_
#define NETWORK_CHECK_H_



/** 是否开始检测网络标志 */
extern volatile int g_nNetCheckFlag;
/** 是否开始重设网络标志 */
extern volatile int g_nResetNetFlag;
/** 开始dhcp */
extern volatile int g_nStartDhcpFlag;
/** DHCP 已OK */
extern volatile int g_nDhcpAlreadyOk;
/** 网络check线程阻塞标志 */
extern volatile int g_nNetCheckBlockFlag;

extern char g_acGateway[16];

extern unsigned int g_nDhcpFinishTime;

/** 以下变量用于测试  */
extern unsigned int g_nNetIwprivEssidoff;
extern unsigned int g_nNetIwprivFuncSet;
extern unsigned int g_nNetGetWifiSignal;
extern unsigned int g_nNetConnStatus;
extern unsigned int g_nPopenFlag;

/**
  @brief set网络检测标志
  @author wfb
  @param[in] 无
  @param[out] 无
  @return 无
  @remark 2013-11－13 增加该函数
  @note
*/
void set_net_check_flag();

/**
  @brief clear网络检测标志
  @author wfb
  @param[in] 无
  @param[out] 无
  @return 无
  @remark 2013-11－13 增加该函数
  @note
*/
void clear_net_check_flag();

/**
  @brief set重设网络标志
  @author wfb
  @param[in] 无
  @param[out] 无
  @return 无
  @remark 2013-11－13 增加该函数
  @note
*/
void set_reset_net_flag();

/**
  @brief clear重设网络标志
  @author wfb
  @param[in] 无
  @param[out] 无
  @return 无
  @remark 2013-11－13 增加该函数
  @note
*/
void clear_reset_net_flag();


/**
  @brief set start_dhcp
  @author wfb
  @param[in] 无
  @param[out] 无
  @return 无
  @remark 2013-11-25 增加该函数
  @note
*/
void set_start_dhcp_flag(int nFlag);


/**
  @brief clear start_dhcp
  @author wfb
  @param[in] 无
  @param[out] 无
  @return 无
  @remark 2013-11-25 增加该函数
  @note
*/
void clear_start_dhcp_flag();


/**
  @brief set dhcp_ok
  @author wfb
  @param[in] 无
  @param[out] 无
  @return 无
  @remark 2013-11-25增加该函数
  @note
*/
void set_dhcp_ok_flag();


/**
  @brief clear dhcp_ok
  @author wfb
  @param[in] 无
  @param[out] 无
  @return 无
  @remark 2013-11-25 增加该函数
  @note
*/
void clear_dhcp_ok_flag();


/**
  *  @brief      获取网关
  *  @author     <wfb> 
  *  @param[in]  <无>
  *  @param[out] <无>
  *  @return     <无>
  *  @remark     2013-09-03 增加该注释
  *  @note
*/
int get_gateway(char *pacGateWay);


/**
  *  @brief      获取ip地址
  *  @author     <wfb> 
  *  @param[in]  <pcIpAddr: IP地址>
  *  @param[in]  <nNetType: 网络类型>
  *  @param[out] <无>
  *  @return     <EXIT_FAILURE EXIT_SUCCESS>
  *  @remark     2013-09-03 增加该注释
  *  @note
*/
void get_ip_addr(char *pcIpAddr, int nNetType);


/**
  @brief set_net_check_block_flag
  @author wfb
  @param[in] 无
  @param[out] 无
  @return 无
  @remark 2013-11-25增加该函数
  @note
*/
void set_net_check_block_flag();

/**
  @brief clear dhcp_ok
  @author wfb
  @param[in] 无
  @param[out] 无
  @return 无
  @remark 2013-11-25 增加该函数
  @note
*/
void clear_net_check_block_flag();


/**
  @brief      网络检测线程实现
  @author     <wfb> 
  @param[in]  <无>
  @param[out] <无>
  @return     <无>
  @remark     2013-11-13 增加该函数
  @note
*/
int NetworkCheck_Thread_Creat(void);

#endif /* THREAD_H_ */
