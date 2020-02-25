#ifndef __RT5350_DEV_H__
#define __RT5350_DEV_H__


//MISC设备注册起始号
#define RT5350_DEV_MINOR_START            (200)


/* 生命设备 minor+index号 */
enum
{
	DEV_INDEX_FIRST,

	DEV_INDEX_IO = DEV_INDEX_FIRST,            /*!< 普通的io口 */

	DEV_INDEX_LAST = DEV_INDEX_IO,
};

/* 驱动结构体 */
typedef struct _tag_rt5350_device_
{
    unsigned char           index;           /*!> 设备索引 */
    unsigned char           opened;          /*!> 已经打开标志 */
    unsigned char           name[15];        /*!> 设备名称 */
    unsigned char           mode;            /*!> 运行模式 */
    volatile unsigned int   state;           /*!> 设备状态  */
    struct file_operations *fops;            /*!> 文件操作 */

    struct timer_list       timer;           /*!> 设备定时器 */
    spinlock_t              lock;            /*!> 设备锁 */
}rt5350_dev_t;

#endif   /*END __RT5350_DEV_H__ */
