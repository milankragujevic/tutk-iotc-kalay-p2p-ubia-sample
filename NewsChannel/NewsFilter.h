/*
 * NewsFilter.h
 *
 *  Created on: Apr 15, 2013
 *      Author: yangguozheng
 */

#ifndef NEWSFILTER_H_
#define NEWSFILTER_H_

enum {
    FILTER_TRUE,    //查找成功, 结束查找
    FILTER_FALSE,   //stop
    FILTER_NEXT,    //continue
};

/*协议查询入口*/
extern int NewsFilter_cmd_table_fun(void *self);

#endif /* NEWSFILTER_H_ */
