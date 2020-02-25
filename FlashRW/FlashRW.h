/**   @file []
   *  @brief    (1)Flash线程 自动更新
   *  @note
   *  @author   wangfengbo
   *  @date     2013-9-3
   *  @remarks  增加注释
*/
#ifndef _FLASH_RW_H_
#define _FLASH_RW_H_


/** flash线程消息命令 */
enum
{
	MSG_FLASHRW_T_WRITE_MACS = 1,
	MSG_FLASHRW_T_READ_MACS,
	MSG_FLASHRW_P_READ_MACS,
	MSG_FLASHRW_T_WRITE_CONFIG_TO_RAM,
	MSG_FLASHRW_T_WRITE_CONFIG_TO_FLASH,
	MSG_FLASHRW_T_READ_CONFIG,
	MSG_FLASHRW_P_READ_CONFIG,
	MSG_FLAHSRW_T_READ_PB_KEY,
	MSG_FLAHSRW_P_READ_PB_KEY,
	MSG_FLAHSRW_T_WRITE_PB_KEY,
	MSG_FLAHSRW_T_UPDATA_CONFIG_TO_FLASH,
};

/**
  *  @brief      FLASH读写线程创建函数
  *  @author     <wfb> 
  *  @param[in]  <无>
  *  @param[out] <无>
  *  @return     <创建线程ID>
  *  @remark     2013-09-03 增加该注释
  *  @note
*/
int flash_wr_thread_creat(void);

#endif /* _FLASH_RW_H_ */
