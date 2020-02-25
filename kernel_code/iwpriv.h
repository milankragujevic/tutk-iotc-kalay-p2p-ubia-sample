/**   @file []
   *  @brief    线程公用函数
   *  @note
   *  @author   wangfengbo
   *  @date     2013-9-2
   *  @remarks  增加注释
*/
#ifndef __IWPRIV_H_
#define __IWPRIV_H_

enum
{
	IWPRIV_CMD_CONNSTATUS = 0x00,
	IWPRIV_CMD_SET,
};



/**
  @brief iwpriv_func
  @author wfb
  @param[in] nCmdType:
                 0: IWPRIV_CMD_CONNSTATUS
                 1: IWPRIV_CMD_SET
             pacName:
                                   这里一般应该是 ra0
             pacParam1：参数1
             pacParam2：参数2
  @param[out] 无
  @return 无
  @remark 2013-11-25 增加该函数
  @note
 */
int iwpriv_func(int nCmdType, char *pacName, char *pacParam1, char *pacParam2);

#endif /* __IWPRIV__H_ */
