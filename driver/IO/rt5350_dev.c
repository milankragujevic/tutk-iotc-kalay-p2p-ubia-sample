/*! \file    rt5350_dev.c
* \b 功能描述
*
*    RT5350 驱动模块
*
* \b 接口描述
*
*
* \b 历史记录:
*
*        <作者>          <时间>          <修改记录>
*      wangfengbo      2013/03/28       创建该文件
*/
/********************************************************************/
/*                              头文件                                   */
/********************************************************************/
#include <linux/init.h>
#include <linux/version.h>
#include <linux/module.h>
#include <linux/config.h>
#include <linux/kernel.h>
#include <linux/interrupt.h>
#include <linux/fs.h>
#include <linux/major.h>
#include <linux/mm.h>
#include <linux/io.h>
#include <linux/ioport.h>
#include <linux/ioctl.h>
//#include <asm/sizes.h>
#include <asm/io.h>
#include <linux/delay.h>
#include <asm/uaccess.h>
#include <linux/capability.h>
#include <linux/vmalloc.h>
#include <asm/pgtable.h>
#include <asm/system.h>
#include <linux/spinlock.h>
#include <linux/err.h>
#include <linux/kernel.h>
#include <linux/types.h>
#include <linux/miscdevice.h>
#include <linux/fcntl.h>
#include <linux/init.h>
#include <linux/sysctl.h>
#include <linux/slab.h>
#include <linux/string.h>
#include <linux/wait.h>
#include <linux/irq.h>
#include <linux/slab.h>
#include <linux/seq_file.h>
#include <linux/proc_fs.h>
//#include <asm/mach-types.h>
#include <asm/irq.h>
#include <asm/uaccess.h>
#include <asm/rt2880/surfboardint.h>
#include <asm/rt2880/rt_mmap.h>
#include <asm/io.h>
#include <asm/system.h>

#include "rt5350_dev.h"
#include "rt5350_ioctl.h"

#ifdef  CONFIG_DEVFS_FS
#include <linux/devfs_fs_kernel.h>
static  devfs_handle_t devfs_handle;
#endif

#define DEBUG   1

/* 调试宏定义 */
#ifdef   DEBUG
#define  DPRINTK(fmt, args...) \
    printk("%s: " fmt, __FUNCTION__, ## args)
#else
#define  DPRINTK(fmt, args...)
#endif


/* 声明设备结构和内部函数 */
#define RT5350_DECLARE_DEV(name)                                                                        \
    static int rt5350_dev_##name##_open(struct inode *inode, struct file *filp);                        \
    static int rt5350_dev_##name##_release(struct inode *inode, struct file *filp);                     \
    static int rt5350_dev_##name##_read(struct file *filp, char *buf, size_t length, loff_t *offset);   \
    static int rt5350_dev_##name##_ioctl(struct inode *inode, struct file *filp, unsigned int cmd, unsigned long arg); \
    static unsigned int rt5350_dev_##name##_poll(struct file *filp, struct poll_table_struct *wait);    \
    static struct file_operations rt5350_dev_##name =                                                   \
{                                                                                                       \
    .owner   = THIS_MODULE,                                                                             \
    .open    = rt5350_dev_##name##_open,                                                                \
    .release = rt5350_dev_##name##_release,                                                             \
    .read    = rt5350_dev_##name##_read,                                                                \
    .ioctl   = rt5350_dev_##name##_ioctl,                                                               \
    .poll    = rt5350_dev_##name##_poll,                                                                \
};


#define	RT5350_REG(x)		         (*((volatile u32 *)(x)))
#define	RT5350_GPIOMODE_REG		 (RALINK_SYSCTL_BASE+0x60)	//it's 6 should be set '1'
#define RT5350_PIOINT_REG		 (RALINK_PIO_BASE)		//Int status register,write 1 to clear int statu
#define RT5350_PIOEDGE_REG		 (RALINK_PIO_BASE+0x04)
#define RT5350_PIORENA_REG		 (RALINK_PIO_BASE+0X08)
#define RT5350_PIOFMASK_REG		 (RALINK_PIO_BASE+0X0C)
#define RT5350_PIODATA_REG		 (RALINK_PIO_BASE+0X20)
#define RT5350_PIODIR_REG		 (RALINK_PIO_BASE+0X24)
#define RT5350_INTENA_REG		 (RALINK_INTCL_BASE+0x34)


/*! add some rt5350 register 这个寄存器还有很多疑问  ？？？？？？？？？ */
#define  M3S_MONITOR_WAN_PORT_REGISTER    (0x10110080)      /*!> 这个使用来检测网络是否插入 第29位 */
#define  M3S_POWER_PORT_REGISTER          (0x10000620)


/********************************************************************/
/*                              内部变量定义                                                  */
/********************************************************************/
/* 声明设备驱动结构 */
/* io口控制驱动  */
RT5350_DECLARE_DEV(io)

/* 设备信息 */
rt5350_dev_t rt5350_dev[] =
{
    {DEV_INDEX_IO,  0, "DEV_IDX_IO",  0, 0, &rt5350_dev_io }, /*!< io设备 */
};

/* 设备数量 */
#define NUM_RT5350_DEVS (sizeof(rt5350_dev)/sizeof(rt5350_dev[0]))

/* 注册时使用的MISC设备数据结构 */
struct miscdevice rt5350_misc_devs[NUM_RT5350_DEVS];


/********************************************************************/
/*                            内部接口实现                                                      */
/********************************************************************/
/*! \fn       static int rt5350_dev_io_open(struct inode *inode, struct file *file)
*  \brief     处理I/O设备打开
*  \param     inode     设备节点
*  \param     file      设备文件
*  \exception
*  \return    0:OK, -1:Failed
*  \b history:
*
*     <作者>            <日期>           <修改记录> <br>
*     wangfengbo       2013-03-28        创建该函数
*/
static int rt5350_dev_io_open(struct inode *inode, struct file *filp)
{
    rt5350_dev_t *pstIoDev = &(rt5350_dev[DEV_INDEX_IO]);

    DPRINTK("open io ok");
    pstIoDev->lock = SPIN_LOCK_UNLOCKED;
    spin_lock_init(&(pstIoDev->lock));

    spin_lock(&(pstIoDev->lock));

    if (0 != pstIoDev->opened)
    {
    	DPRINTK("want to open io dev, not allowd busy now");
    	spin_unlock(&(pstIoDev->lock));
    	return -EBUSY;
    }

    /* 设置设备已经打开标志 */
    pstIoDev->opened++;
    spin_unlock(&(pstIoDev->lock));

    return 0;
}


/*! \fn       static int rt5350_dev_io_release(struct inode *inode, struct file *filp)
*  \brief     处理I/O设备释放
*  \param     inode     设备节点
*  \param     file      设备文件
*  \exception
*  \return    0:OK, -1:Failed
*  \b history:
*
*     <作者>            <日期>           <修改记录> <br>
*     wangfengbo       2013-03-28        创建该函数
*/
static int rt5350_dev_io_release(struct inode *inode, struct file *filp)
{
    rt5350_dev_t *pstIoDev = &(rt5350_dev[DEV_INDEX_IO]);

    DPRINTK("release dev io ok");

    /* 清空设备已经打开标志 */
    pstIoDev->opened = 0;

    return 0;
}

/*! \fn       static int rt5350_dev_io_read(struct file *filp, char *buf, size_t length, loff_t *offset)
*  \brief     读I/O设备
*  \param     inode     设备节点
*  \param     file      设备文件
*  \exception
*  \return    0:OK, -1:Failed
*  \b history:
*
*     <作者>            <日期>           <修改记录> <br>
*     wangfengbo       2013-03-28        创建该函数
*/
static int rt5350_dev_io_read(struct file *filp, char *buf, size_t length, loff_t *offset)
{
    return 0;
}


/*! \fn       static int rt5350_dev_io_ioctl(struct inode *inode, struct file *file, unsigned int cmd, unsigned long arg)
*  \brief     io设备控制命令
*  \param     inode   设备节点
*  \param     file    设备文件
*  \param     cmd     设备命令
*  \param     arg     命令参数
*  \exception 无
*  \return    0:成功  -1：失败
*  \b 历史记录:
*
*     <作者>        <时间>       <修改记录> <br>
*     wangfengbo  2013-03-28    创建该函数 <br>
*/
static int rt5350_dev_io_ioctl(struct inode *inode, struct file *file, unsigned int cmd, unsigned long arg)
{
    unsigned int ulData = 0;
    u32 reg_value = 0;
    int ret = 0;
    //rt5350_dev_t *pstIoDev = &(rt5350_dev[DEV_INDEX_IO]);


    /*! 检查magic */
    if( _IOC_TYPE(cmd) != RT5350_IOCTL_MAGIC )
        return -ENOTTY;

    /*! 如果是往设备写，则需要事先从用户空间拷贝数据 */
    if( _IOC_WRITE & _IOC_DIR(cmd) )
    {
        /*! 从用户空间拷贝数据 */
        __get_user(ulData, (int *)arg);
    }

    /*! 命令分类 */
    switch(cmd)
    {
    case CMD_GET_USB_STATE:
    	reg_value = RT5350_REG(RT5350_PIODATA_REG);
    	//DPRINTK("reg_value is 0x%x\n", reg_value);
        //printk("come into get cmd_get_usb_state");
    	if (reg_value & (0x01<<19))
    	{
           // DPRINTK("device not connect\n");
            /* no device*/
    	    ret = DEVICE_NOT_CONNECT;
        }
    	else
    	{
    	    /* have device*/
            //DPRINTK("device connect\n");
    	    ret = DEVICE_CONNECT;
    	}
    	break;
    case CMD_GET_NET_STATE:
    	reg_value = inl(M3S_MONITOR_WAN_PORT_REGISTER);

    	if (reg_value & (1<<29))
    	{
    		ret = DEVICE_CONNECT;
    		//DPRINTK("NET IS CONNECT");
    	}
    	else
    	{
    		ret = DEVICE_NOT_CONNECT;
    		//DPRINTK("NET IS NOT CONNECT");
    	}
    	break;
    case CMD_POWERON_USB:
    	RT5350_REG(RT5350_PIODATA_REG) |= 0x01<<21;
        break;
    case CMD_POWEROFF_USB:
    	RT5350_REG(RT5350_PIODATA_REG) &= ~(0x01<<21);
        break;
    case CMD_SWITCH_TO_MFI:
    	RT5350_REG(RT5350_PIODATA_REG) |= 0x01<<18;  //link to apple device
        break;
    case CMD_SWITCH_TO_CAM:
    	RT5350_REG(RT5350_PIODATA_REG) &= ~(0x01<<18);	//link to camera device
    	break;
    case CMD_NET_LIGHT_OFF:
        DPRINTK("kenerl: net light on");
    	reg_value = inl(0x1000062c);
    	reg_value |=(0x01<<13);
    	outl(reg_value, 0x1000062c);
        break;
    case CMD_NET_LIGHT_ON:
        DPRINTK("kenerl: net light off");
    	reg_value = inl(0x10000630);
    	reg_value |=(0x01<<13);
    	outl(reg_value, 0x10000630);
        break;
    case CMD_POWER_LIGHT_ON:
        DPRINTK("kenerl: power light on");
    	reg_value = inl(0x10000620);
    	reg_value &=~(0x1<<12);
    	outl(reg_value, 0x10000620);
    	break;
    case CMD_POWER_LIGHT_OFF:
        DPRINTK("kenerl: power light on");
        reg_value = inl(0x10000620);
        reg_value |= (0x1<<12);
        outl(reg_value, 0x10000620);
        break;
    case CMD_NIGHT_LIGHT_ON:
	DPRINTK("NIGHT_ON\n");
	reg_value = inl(0x1000062c);
	reg_value |= (0x1<<11);
	outl(reg_value, 0x1000062c);
	break;
    case CMD_NIGHT_LIGHT_OFF:
	DPRINTK("NIGHT_OFF\n");
	reg_value = inl(0x10000630);
	reg_value |=(0x1<<11);
	outl(reg_value, 0x10000630);
	break;
    default:
        return -EFAULT;
    }

    /*! 如果是从设备读取，则要把读取的数据传送到用户空间 */
    if( _IOC_READ & _IOC_DIR(cmd) )
        __put_user(ret, (int *)arg);

    return 0;
}

/*! \fn       static unsigned int rt5350_dev_io_poll(struct file *filp, struct poll_table_struct *wait)
*  \brief     poll设备等待数据
*  \param     file   设备文件描述
*  \param     wait   等待列表
*  \exception 无
*  \return    0:无数据   POLLIN | POLLRDNORM:有数据  POLLERR:错误
*  \b 历史记录:
*
*     <作者>        <时间>      <修改记录> <br>
*     qinjunfang  2010-11-23    创建该函数 <br>
*/
static unsigned int rt5350_dev_io_poll(struct file *filp, struct poll_table_struct *wait)
{
    return 0;
}

/*! 初始化gpio */
void m3s_rt5350_pheripherial_init(void)
{   
    unsigned int reg_val;

    reg_val=inl(0x10000060); //set gpiomodule
    reg_val&=~(0x07<<2);
    reg_val|=0x06<<2;
    outl(reg_val,0x10000060);

    reg_val=inl(0x10000624); //set dir
    reg_val|=0x07<<11;
    outl(reg_val,0x10000624);

    reg_val=inl(0x10000630);//clear corresponding bit
    reg_val|=0x01<<12;
    outl(reg_val,0x10000630);
}

/* 释放gpio */
void m3s_rt5350_pheripherial_deinit(void)
{
    return;
}

int __init rt5350_io_init(void)
{
    int lDevCount = 0;

    printk("************************************************\n");
    printk("*************come into 5350**********************\n");
    printk("************************************************\n");
    
    /*! 初始化gpio模块 */
    m3s_rt5350_pheripherial_init();

#if  10  //移植过来的 再修改
	RT5350_REG(RT5350_GPIOMODE_REG) |= 0x01<<6;		//set JATG TO GPIO_MODE
	RT5350_REG(RT5350_PIODIR_REG) |= 0x01<<18;	    //GPIO#18 set as Output
	RT5350_REG(RT5350_PIODIR_REG) |= 0x01<<21;	    //GPIO#21 set as Output
	RT5350_REG(RT5350_PIODIR_REG) &= ~(0x01<<19);	//GPIO#19 set as input
	RT5350_REG(RT5350_PIODATA_REG) &= ~(0x01<<18);	//low level at GPIO#18
	RT5350_REG(RT5350_PIODATA_REG) |= 0x01<<21;	    //High level at GPIO#21
	wmb();
	//RT5350_REG(RT5350_PIODATA_REG);

#endif
    /* 注册设备 */
    for (lDevCount = 0; lDevCount < NUM_RT5350_DEVS; lDevCount++)
    {
        /* minor */
    	rt5350_misc_devs[lDevCount].minor = rt5350_dev[lDevCount].index + RT5350_DEV_MINOR_START;
        /* name */
    	rt5350_misc_devs[lDevCount].name  = rt5350_dev[lDevCount].name;
        /* fops */
    	rt5350_misc_devs[lDevCount].fops  = rt5350_dev[lDevCount].fops;
    	rt5350_dev[lDevCount].opened = 0;

        /* 注册设备 */
        if( 0 != misc_register(&(rt5350_misc_devs[lDevCount])) )
        {
            DPRINTK("register misc device: %s error \n", rt5350_misc_devs[lDevCount].name);
            return -EBUSY;
        }
     }

     return 0;
}

void __exit rt5350_io_exit(void)
{
    int lDevCount = 0;

    /* 释放gpio */
    m3s_rt5350_pheripherial_deinit();
    /* 卸载设备  */
    for (lDevCount = 0; lDevCount < NUM_RT5350_DEVS; lDevCount++)
    {
        misc_deregister(&(rt5350_misc_devs[lDevCount]));
    }
  
    RT5350_REG(RT5350_PIODATA_REG) &= ~(0X01<<18); 
    DPRINTK("RT5350 io driver exited\n");

    return;
}

module_init(rt5350_io_init);
module_exit(rt5350_io_exit);


MODULE_DESCRIPTION("M3S RT5350 Driver");
MODULE_AUTHOR("wangfengbo@jiuan.com");
MODULE_LICENSE("GPL");

