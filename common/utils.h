#ifndef UTILS_H_
#define UTILS_H_
#include <stdint.h>
#include <stdlib.h>

#define MSG_MAX_LEN 1024

/*模块查找描述*/
typedef struct type_item {
	long int type;
	void *data;
}type_item;

/*协议查找描述*/
typedef struct cmd_item {
	int cmd;
	void *data;
}cmd_item;

/*适配器基本运行环境描述*/
typedef struct msg_container {
	int lmsgid;
	int rmsgid;
	char msgdata_rcv[MSG_MAX_LEN];
	char msgdata_snd[MSG_MAX_LEN];
}msg_container;

/*动作描述*/
typedef void (*msg_fun)(msg_container *self);

/*ascii码(16进制)转为整形, 单字符*/
extern inline int utils_atoi_a(char *to, const char *from);
/*ascii码(16)转为整形, 双字符*/
extern inline int utils_atoi_b(char *to, const char *from);
/*ascii码(16)转为整形, size*2个字符*/
extern inline int utils_astois(char *to, const char *from, int size);
/*整形转为ascii码(16), size*2个字符*/
extern inline int utils_istoas(char *to, const char *from, int size);

/*xxtea 对称加密*/
extern void utils_btea(uint32_t *v, int n, uint32_t const key[4]);

/*屏蔽标准输出*/
extern int set_disp_mode(int mode);

/*查询wifi列表*/
extern int utils_searchWifiList(char *wifiList, int size);

/*模块查找函数*/
extern void *utils_type_bsearch(type_item *type, type_item *type_table, size_t size);

/*协议查找函数*/
extern void *utils_cmd_bsearch(cmd_item *cmd, cmd_item *cmd_table, size_t size);
#endif /* UTILS_H_ */
