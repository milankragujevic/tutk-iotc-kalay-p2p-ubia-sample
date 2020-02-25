/**   @file []
   *  @brief    网路子进程实现
   *  @note
   *  @author   wangfengbo
   *  @date     2013-9-2
   *  @remarks  增加注释
*/

#ifndef __NETSET_ADAPTER_H_
#define __NETSET_ADAPTER_H_



/**
  *  @brief      网络子进程消息处理函数
  *  @author     <wfb> 
  *  @param[in]  <pstMsgContainer: 接收子进程消息>
  *  @param[out] <无>
  *  @return     <无>
  *  @remark     2013-09-03 增加该注释
  *  @note
*/
void net_cmd_table_fun(msg_container *pstMsgContainer);
#endif /* __NETSET_ADAPTER_H_ */
