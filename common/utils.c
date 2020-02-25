/**	
   @file utils.c
   @brief 工具函数
   @note
   author: yangguozheng
   date: 2013-09-16
   mcdify record: creat this file.
   author: yangguozheng
   date: 2013-09-17
   modify record: add a function
 */
#include "common/utils.h"
#include "common/logfile.h"
#include <stdint.h>
#include <termios.h>
#include <unistd.h>
#include <stdlib.h>
#include <regex.h>


#define ECHOFLAGS (ECHO | ECHOE | ECHOK | ECHONL) //< 调试常量

#define DELTA 0x9e3779b9 //< 加密
#define MX (((z>>5^y<<2) + (y>>3^z<<4)) ^ ((sum^y) + (key[(p&3)^e] ^ z))) //< 加密宏

/**
   @brief assic字符转换为整形
   @author yangguozheng 
   @param[out] to 目标 
   @param[in] from 源字符 
   @return 0
   @note
 */
inline int utils_atoi_a(char *to, const char *from) {
	if(*from > 'F' || *from < '0') {
		return -1;
	}
	if(*from > 'A' - 1) {
		*to = *from - 'A' + 10;
	}
	else if(*from > '0' - 1) {
		*to = *from - '0';
	}
//	DEBUGPRINT(DEBUG_INFO, "<>--[%d]--<>\n", *to);
	return 0;
}

/**
   @brief assic双字符转换为整形
   @author yangguozheng 
   @param[out] to 目标 
   @param[in] from 源双字符 
   @return 0
   @note
 */
inline int utils_atoi_b(char *to, const char *from) {
	char b = 0;
	if(utils_atoi_a(&b, from) == -1) {
		return -1;
	}
	*to = 0;
	*to += b * 16;
	if(utils_atoi_a(&b, from + 1) == -1) {
		return -1;
	}
	*to += b;
	return 0;
}

/**
   @brief assic字符串转换为整形
   @author yangguozheng 
   @param[out] to 目标 
   @param[in] from 源字符串，必须为偶数个 
   @return 0
   @note
 */
inline int utils_astois(char *to, const char *from, int size) {
	int i = 0;
	for(;i < size;++i) {
		if(utils_atoi_b(to, from) == -1) {
			return -1;
		}
//		DEBUGPRINT(DEBUG_INFO, "<>--[%d]--<>\n", *to);
		++to;
		from += 2;
	}
	return 0;
}

/**
   @brief 半字节整形转换为assic字符
   @author yangguozheng 
   @param[out] to 目标 
   @param[in] from 源整形 
   @return 0
   @note
 */
inline int utils_itoa_a(char *to, const char from) {
	if(from > 0x09) {
		*to = 'A' + from - 0x0A;
	}
	else {
		*to = '0' + from - 0x00;
	}
	return 0;
}

/**
   @brief 一字节整形转换为assic双字符
   @author yangguozheng 
   @param[out] to 目标 
   @param[in] from 源整形 
   @return 0
   @note
 */
inline int utils_itoa_b(char *to, const char from) {
	utils_itoa_a(to, (from&0xFF)>>4);
	utils_itoa_a(to + 1, from&0x0F);
	return 0;
}

/**
   @brief 整形转换为assic字符串
   @author yangguozheng 
   @param[out] to 目标 
   @param[in] from 源整形 
   @return 0
   @note
 */
inline int utils_istoas(char *to, const char *from, int size) {
	int i = 0;
	for(;i < size;++i) {
		utils_itoa_b(to, *from);
		to += 2;
		++from;
	}
	return 0;
}

/**
   @brief xxtea加密解密
   @author yangguozheng 
   @param[in|out] v 随机数 
   @param[in] n 长度，负数为解密 
   @param[in] key 密钥
   @return 0
   @note
 */
 void utils_btea(uint32_t *v, int n, uint32_t const key[4]) {
   uint32_t y, z, sum;
   unsigned p, rounds, e;
   if (n > 1) {          /* Coding Part */
     rounds = 6 + 52/n;
     sum = 0;
     z = v[n-1];
     do {
       sum += DELTA;
       e = (sum >> 2) & 3;
       for (p=0; p<n-1; p++) {
         y = v[p+1];
         z = v[p] += MX;
       }
       y = v[0];
       z = v[n-1] += MX;
     } while (--rounds);
   } else if (n < -1) {  /* Decoding Part */
     n = -n;
     rounds = 6 + 52/n;
     sum = rounds*DELTA;
     y = v[0];
     do {
       e = (sum >> 2) & 3;
       for (p=n-1; p>0; p--) {
         z = v[p-1];
         y = v[p] -= MX;
       }
       z = v[n-1];
       y = v[0] -= MX;
     } while ((sum -= DELTA) != 0);
   }
 }

/**
   @brief 设置应用模式，用于关闭输入回显
   @author yangguozheng 
   @param[in] mode 模式 
   @return 成功: 0，失败: -1
   @note
 */
int set_disp_mode(int mode) {
	struct termios term;
	if(tcgetattr(STDIN_FILENO, &term) == -1) {
		DEBUGPRINT(DEBUG_ERROR, "tcgetattr");
		return -1;
	}
	term.c_lflag &= ~ECHOFLAGS;

	int res = tcsetattr(STDIN_FILENO, TCSAFLUSH, &term);
	if(res == -1 && res == EINTR) {
		DEBUGPRINT(DEBUG_ERROR, "tcsetattr");
		return -1;
	}
	return 0;
}

/**
   @brief 查询wifi列表
   @author yangguozheng 
   @param[out] wifiList wifi列表 
   @param[in] size 大小，未使用
   @return 成功: 0，失败: -1
   @note
 */
int utils_searchWifiList(char *wifiList, int size) {
	FILE *file;
	system("iwlist ra0 scanning > /tmp/wifilist");
	file = fopen("/tmp/wifilist", "r");

	if(file == NULL) {
		return -1;
	}
	char buf[512];
	regex_t re, re_qu;
	regmatch_t match[1], match_qu[1];
	regcomp(&re, "ESSID:\"[^\"]*\"", REG_EXTENDED);
	regcomp(&re_qu, "Quality=[0-9]{1,3}/100", REG_EXTENDED);

	while(NULL != fgets(buf, sizeof(buf), file)) {
		//DEBUGPRINT(DEBUG_INFO, "y :%s\n", buf);
		if(0 == regexec(&re, buf, 1, match, 0)) {
			//DEBUGPRINT(DEBUG_INFO, "y :%s\n", buf + match[0].rm_so);
			strncat(wifiList, buf + match[0].rm_so, match[0].rm_eo - match[0].rm_so);
			strcat(wifiList, " ");
		}
		else {
			if(0 == regexec(&re_qu, buf, 1, match_qu, 0)) {
				//DEBUGPRINT(DEBUG_INFO, "y :%s\n", buf + match_qu[0].rm_so);
				strncat(wifiList, buf + match_qu[0].rm_so, match_qu[0].rm_eo - match_qu[0].rm_so);
				strcat(wifiList, "\n");
			}
		}
	}
	return 0;
}

//typedef void (*msg_fun)(char *msg, int msgId);

/**
   @brief 二分法查找对比函数，用于消息队列中消息类型对比
   @author yangguozheng 
   @param[in] left 左值 
   @param[in] right 右值
   @return 对比结果
   @note
 */
int utils_type_compar(const void *left, const void *right) {
	type_item *a = (type_item *)left;
	type_item *b = (type_item *)right;
	return a->type - b->type;
}

/**
   @brief 二分法查找对比函数，用于消息队列中消息命令对比
   @author yangguozheng 
   @param[in] left 左值 
   @param[in] right 右值
   @return 对比结果
   @note
 */
int utils_cmd_compar(const void *left, const void *right) {
	cmd_item *a = (cmd_item *)left;
	cmd_item *b = (cmd_item *)right;
	return a->cmd - b->cmd;
}

/**
   @brief 二分法查找函数，用于消息队列中消息类型查找
   @author yangguozheng 
   @param[in] type 消息类型 
   @param[in] type_table 协议表
   @param[in] count 表大小，项目条数
   @return 对比结果
   @note
 */
void *utils_type_bsearch(type_item *type, type_item *type_table, size_t count) {
	type_item *itype = bsearch(type, type_table, count, sizeof(type_item), utils_type_compar);
	if(itype == NULL) {
		return NULL;
	}
	return itype->data;
}

/**
   @brief 二分法查找函数，用于消息队列中消息类型查找
   @author yangguozheng 
   @param[in] type 消息类型 
   @param[in] type_table 协议表
   @param[in] count 表大小，项目条数
   @return 对比结果
   @note
 */
void *utils_cmd_bsearch(cmd_item *cmd, cmd_item *cmd_table, size_t count) {
	cmd_item *icmd = bsearch(cmd, cmd_table, count, sizeof(cmd_item), utils_cmd_compar);
	if(icmd == NULL) {
		return NULL;
	}
	return icmd->data;
}
