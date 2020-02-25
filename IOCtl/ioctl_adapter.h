/**   @file []
   *  @brief    IO子进程
   *  @note
   *  @author   wangfengbo
   *  @date     2013-9-2
   *  @remarks  增加注释
*/

#ifndef __IOCTL_ADAPTER_H_
#define __IOCTL_ADAPTER_H_
#include "common/utils.h"


/**
  *  @brief      查找cmd对应的处理函数
  *  @author     <wfb> 
  *  @param[in]  <pstMsgContainer: 接受消息>
  *  @param[out] <无>
  *  @return     <无>
  *  @remark     2013-09-02 增加该注释
  *  @note
*/
void ioctl_cmd_table_fun(msg_container *pstMsgContainer);

#endif /* __IOCTL_ADAPTER_H_ */
