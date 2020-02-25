/**	
   @file msg_queue.c
   @brief 消息队列公用函数
   @note
   author: yangguozheng
   date: 2013-09-16
   mcdify record: creat this file.
   author: yangguozheng
   date: 2013-09-17
   modify record: add a function
 */
#include "common/msg_queue.h"
#include "common/logfile.h"
#include "common/common.h"

/**
   @brief 创建消息队列
   @author yangguozheng 
   @param[in] key 消息队列键值，唯一 
   @return id>0:队列id, -1:失败
   @note
     例子：msg_queue_create(1234);
 */
int msg_queue_create(int key) {
#if _MSG_QUEUE_VERSION == 2
	return msg_queue_create_a(key);
#else
	int id = msgget(key, 0660 | IPC_CREAT);
	if(id == -1) {
		DEBUGPRINT(DEBUG_ERROR, "%s: %s\n", __FUNCTION__, errstr);
	}
	return id;
#endif
}

/**
   @brief 通过队列id删除消息队列
   @author yangguozheng 
   @param[in] msgid 队列id，可以通过msg_queue_get获取 
   @return 0:成功，-1:失败
   @note
 */
int msg_queue_remove(int msgid) {
	return msgctl(msgid, IPC_RMID, 0);
}

/**
   @brief 通过队列键值删除消息队列
   @author yangguozheng 
   @param[in] key 队列键值 
   @return 0:成功，-1:失败
   @note
 */
int msg_queue_remove_by_key(int key) {
#if _MSG_QUEUE_VERSION == 2
	return msg_queue_remove_by_key_a(key);
#else
	int msgid = msgget(key, IPC_EXCL);
	if(msgid == -1) {
		DEBUGPRINT(DEBUG_ERROR, "%s: %s\n", __FUNCTION__, errstr);
		return -1;
	}
	return msgctl(msgid, IPC_RMID, 0);
#endif
}

/**
   @brief 消息队列控制
   @author yangguozheng 
   @param[in] msqid 队列id 
   @param[in] cmd 命令 
   @param[in|out] buf 详细配置 
   @return 0:成功，-1:失败
   @note
 */
int msg_queue_ctl(int msqid, int cmd, struct msqid_ds *buf) {
	return msgctl(msqid, cmd, buf);
}

/**
   @brief 获取消息队列
   @author yangguozheng 
   @param[in] key 队列键值 
   @return id>0:队列id, -1:失败
   @note
 */
int msg_queue_get(int key) {
#if _MSG_QUEUE_VERSION == 2
	return msg_queue_get_a(key);
#else
	return msgget(key, 0660 | IPC_CREAT);
#endif
}

/**
   @brief 接收消息
   @author yangguozheng 
   @param[in] msqid 队列id 
   @param[out] msg_ptr 数据缓冲区指针，用于提取数据 
   @param[in] msg_sz 接收消息数据大小 
   @param[in] msgtype 消息类型 
   @return n, n >= 0:成功，-1:失败
   @note
     不阻塞
 */
int msg_queue_rcv(int msqid, void *msg_ptr, size_t msg_sz, long int msgtype) {
	int ret = msgrcv(msqid, msg_ptr, msg_sz, msgtype, IPC_NOWAIT);
	if(-1 != ret) {
		MsgData *data = (MsgData *)msg_ptr;
		log_printf("mqrcv:%d, %ld, %d", msqid, data->type, data->cmd);
		LOG_TEST("msg_queue_rcv time: %08X, id: %d, type: %ld, cmd: %d\n", time(NULL), msqid, data->type, data->cmd);
	}
	return ret;
}

/**
   @brief 发送消息
   @author yangguozheng 
   @param[in] msqid 队列id 
   @param[in] msg_ptr 数据缓冲区指针，指向要发送的数据
   @param[in] msg_sz 发送消息数据大小  
   @return 0:成功，-1:失败
   @note
     不阻塞
 */
int msg_queue_snd(int msqid, const void *msg_ptr, size_t msg_sz) {
	int res = msgsnd(msqid, msg_ptr, msg_sz, IPC_NOWAIT);
	if(res == -1) {
		DEBUGPRINT(DEBUG_ERROR, "%s: %s\n", __FUNCTION__, errstr);
	}
	if(-1 != res) {
		MsgData *data = (MsgData *)msg_ptr;
		log_printf("mqsnd:%d, %ld, %d", msqid, data->type, data->cmd);
		LOG_TEST("msg_queue_snd time: %08X, id: %d, type: %ld, cmd: %d\n", time(NULL), msqid, data->type, data->cmd);
	}
//	MsgData *msg = (MsgData *)msg_ptr;
	//DEBUGPRINT(DEBUG_INFO, "y y y y y y%s: msqid: %d, msg->type: %d\n", __FUNCTION__, msqid, msg->type);
	return res;
}

/**
   @brief 创建消息队列的底层新接口
   @author yangguozheng 
   @param[in] key_id 队列id，唯一  
   @return 0:成功，-1:失败
   @note
     不阻塞
 */
int msg_queue_create_a(int key_id) {
	key_t key = msg_queue_create_key(key_id);
	if(key == -1) {
		return -1;
	}
	int res = msgget(key, 0660 | IPC_CREAT);
	if(res == -1) {
		DEBUGPRINT(DEBUG_ERROR, "%s: %s\n", __FUNCTION__, errstr);
	}
	return res;
}

/**
   @brief 删除消息队列的底层新接口
   @author yangguozheng 
   @param[in] key_id 队列id，唯一  
   @return 0:成功，-1:失败
   @note
     不阻塞
 */
int msg_queue_remove_by_key_a(int key_id) {
	key_t key = msg_queue_create_key(key_id);

	if(key == -1) {
		return -1;
	}
	int msgid = msgget(key, IPC_EXCL);
	if(msgid == -1) {
		DEBUGPRINT(DEBUG_ERROR, "%s: %s\n", __FUNCTION__, errstr);
		return -1;
	}
	int res = msgctl(msgid, IPC_RMID, 0);
	if(res == -1) {
		DEBUGPRINT(DEBUG_ERROR, "%s: %s\n", __FUNCTION__, errstr);
	}
	return res;
}

/**
   @brief 获取消息队列句柄底层新接口
   @author yangguozheng 
   @param[in] key_id 队列id，唯一  
   @return other:队列操作句柄，-1:失败
   @note
     不阻塞
 */
int msg_queue_get_a(int key_id) {
	key_t key = msg_queue_create_key(key_id);
	if(key == -1) {
		return -1;
	}
	int res = msgget(key, 0660 | IPC_CREAT);
	if(res == -1) {
		DEBUGPRINT(DEBUG_ERROR, "%s: %s\n", __FUNCTION__, errstr);
	}
	return res;
}

/**
   @brief 创建消息队列标识的底层新接口
   @author yangguozheng 
   @param[in] key_id 队列id，唯一  
   @return other:队列键值，-1:失败
   @note
     不阻塞
 */
key_t msg_queue_create_key(int key_id) {
	key_t key;
	key = ftok(".", key_id);
	if(key == -1) {
		DEBUGPRINT(DEBUG_ERROR, "%s: %s\n", __FUNCTION__, errstr);
	}
	return key;
}
