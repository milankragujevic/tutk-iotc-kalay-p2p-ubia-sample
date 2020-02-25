#ifndef MSG_QUEUE_H_
#define MSG_QUEUE_H_
#include <sys/msg.h>
#include <sys/types.h>
#include <sys/ipc.h>
#include <sys/socket.h>
#include <arpa/inet.h>

/*interface for message queue*/
/**1
 * 创建消息队列
 * key： key_t，消息队列键值，唯一
 * 返回：{id>0:队列id, -1:失败}
 * 例子：msg_queue_create(1234);
 */
extern int msg_queue_create(key_t key);

/**1
 * 通过队列id删除消息队列
 * msgid：int, 队列id，可以通过msg_queue_get获取
 * 返回：｛0:成功，-1:失败｝
 */
extern int msg_queue_remove(int msqid);

/**1
 * 通过队列键值删除消息队列
 * key: key_t, 队列键值
 * 返回:｛0:成功，-1:失败｝
 */
extern int msg_queue_remove_by_key(key_t key);

/**0
 * 消息队列控制
 * msqid:int, 队列id
 * cmd:int, 命令
 * buf:msqid_ds, 详细配置
 * 返回:｛0:成功，-1:失败｝
 */
extern int msg_queue_ctl(int msqid, int cmd, struct msqid_ds *buf);

/**3
 * 获取消息队列
 * key: key_t, 队列键值
 * 返回：{id>0:队列id, -1:失败}
 */
extern int msg_queue_get(key_t key);

/**5
 * 接收消息
 * msqid:int, 队列id
 * msg_ptr:void*, 数据缓冲区指针，用于提取数据
 * msg_sz:size_t, 接收消息数据大小
 * msgtype:long int, 消息类型
 * 返回:｛n, n >= 0:成功，-1:失败｝
 * 阻塞:N
 */
extern int msg_queue_rcv(int msqid, void *msg_ptr, size_t msg_sz, long int msgtype);

/**5
 * 接收消息
 * msqid:int, 队列id
 * msg_ptr:void*, 数据缓冲区指针，指向要发送的数据
 * msg_sz:size_t, 发送消息数据大小
 * 返回:｛0:成功，-1:失败｝
 * 阻塞:N
 */
extern int msg_queue_snd(int msqid, const void *msg_ptr, size_t msg_sz);

#if 0
extern int msg_queue_create_a(int key_id);
extern int msg_queue_get_a(int key_id);
extern int msg_queue_remove_by_key_a(int key_id);
extern key_t msg_queue_create_key(int key_id);
#endif

#endif /* MSG_QUEQUE_H_ */
