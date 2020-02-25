#include <linux/init.h>
#include <linux/version.h>
#include <linux/config.h>
#include <linux/module.h>
#include <linux/kernel.h> /* printk() */
#include <linux/slab.h> /* kmalloc() */
#include <linux/fs.h> /* everything... */
#include <linux/errno.h> /* error codes */
#include <linux/types.h> /* size_t */
#include <linux/proc_fs.h>
#include <linux/fcntl.h> /* O_ACCMODE */
#include <asm/system.h> /* cli(), *_flags */
#include <asm/uaccess.h> /* copy_from/to_user */
#include <linux/interrupt.h>
#include <linux/mm.h>
#include <asm/rt2880/surfboardint.h>
#include <linux/pci.h>
#include "../ralink_gdma.h"
#include "../ralink_gpio.h"
#include "i2s_ctrl.h"
#include "wm8750.h"

static int i2sdrv_major =  234;
static struct class *i2smodule_class;
extern void audiohw_set_frequency(int fsel);

irqreturn_t i2s_irq_isr(int irq, void *irqaction);//i2s中断服务函数
void i2s_dma_handler(u32 dma_ch); 
void i2s_unmask_handler(u32 dma_ch);

static int i2s_ioctl(struct inode *inode, struct file *file, unsigned int cmd, unsigned long arg);
static int i2s_mmap(struct file *file, struct vm_area_struct *vma);
static int i2s_open(struct inode *inode, struct file *file);
static int i2s_release(struct inode *inode, struct file *file);

i2s_config_type* pi2s_config; //struct i2s_config_t
i2s_status_type* pi2s_status; //struct i2s_status_t
static dma_addr_t i2s_txdma_addr;//dma address
static dma_addr_t i2s_mmap_addr[MAX_I2S_PAGE*2]; //page

static const struct file_operations i2s_fops = 
{
	owner		: THIS_MODULE,
	mmap		: i2s_mmap,
	open		: i2s_open,
	release		: i2s_release,
	ioctl		: i2s_ioctl,	
};

int __init i2s_mod_init(void)
{
    int result=0;
    result = register_chrdev(i2sdrv_major, I2SDRV_DEVNAME, &i2s_fops);
    if (result < 0) 
    {
        return result;
    }

    if (i2sdrv_major == 0)
    {
				i2sdrv_major = result; /* dynamic */
    }

/**调用class_create为该设备创建一个class，
*@再为每个设备调用 class_device_create创建对应的设备
*/
	i2smodule_class=class_create(THIS_MODULE, I2SDRV_DEVNAME);
	if (IS_ERR(i2smodule_class)) 
	{
			return -EFAULT;
	}
	device_create(i2smodule_class, NULL, MKDEV(i2sdrv_major, 0), I2SDRV_DEVNAME);
	return 0;
}

void i2s_mod_exit(void)
{
	device_destroy(i2smodule_class,MKDEV(i2sdrv_major, 0));
	class_destroy(i2smodule_class); 
	return ;
}


static int i2s_open(struct inode *inode, struct file *filp)
{
	int Ret;
	/**
	*@iminor == MINOR(inode->i_rdev), 获取次设备号
	*/
	int minor = iminor(inode);
	if (minor >= I2S_MAX_DEV)
	{
			return -ENODEV;
	}
	try_module_get(THIS_MODULE);//用于增加模块使用计数；若返回为0，表示调用失败，希望使用的模块没有被加载或正在被卸载中。//
	/*应用程序若采用非阻塞方式读取则返回错误*/ 
	if (filp->f_flags & O_NONBLOCK) 
	{
			return -EAGAIN;
	}

	pi2s_config = (i2s_config_type*)kmalloc(sizeof(i2s_config_type), GFP_KERNEL);//分配空间
	if(pi2s_config==NULL)
	{
			return -1;
	}
	filp->private_data = pi2s_config;
	memset(pi2s_config, 0, sizeof(i2s_config_type));//初始化
	
/* pi2s_status */
	pi2s_status = (i2s_status_type*)kmalloc(sizeof(i2s_status_type), GFP_KERNEL);//分配空间
	if(pi2s_status==NULL)
	{
			return -1;
	}
	memset(pi2s_status, 0, sizeof(i2s_status_type));	//初始化
  pi2s_config->srate = 8000;//44100;
  pi2s_config->txvol = 0;
	pi2s_config->flag = 0;
	pi2s_config->dmach = GDMA_I2S_TX0;
	pi2s_config->tx_ff_thres = CONFIG_I2S_TFF_THRES;
	pi2s_config->tx_ch_swap = CONFIG_I2S_CH_SWAP;
	pi2s_config->slave_en = CONFIG_I2S_SLAVE_EN;
	pi2s_config->rxvol = 0;
	pi2s_config->lbk = CONFIG_I2S_INLBK;
	pi2s_config->extlbk = CONFIG_I2S_EXLBK;
	pi2s_config->fmt = CONFIG_I2S_FMT;

  //注册中断服务函
	Ret = request_irq(SURFBOARDINT_I2S, i2s_irq_isr, SA_INTERRUPT, "Ralink_I2S", NULL);
	if(Ret)
	{
			i2s_release(inode, filp);
			return -1;
	}	
  //初始化一个已经存在的等待队列头，它将整个队列设置为"未上锁"状态，并将链表指针prev和next指向它自身。
  init_waitqueue_head(&(pi2s_config->i2s_tx_qh));

	return 0;
}


static int i2s_release(struct inode *inode, struct file *filp)
{
	int i;
	i2s_config_type* ptri2s_config;
	
	/* decrement usage count */
	module_put(THIS_MODULE);//减少模块使用计数
	free_irq(SURFBOARDINT_I2S, NULL);//释放分配给已定中断的内存
	ptri2s_config = filp->private_data;//fs.h
	if(ptri2s_config==NULL)
	{
    	kfree(pi2s_status);/*释放设备结构体内存*/
	    return 0;
	}
	for(i = 0 ; i < MAX_I2S_PAGE*2 ; i ++)
	{
			if(ptri2s_config->pMMAPBufPtr[i])
			{ 
					pci_free_consistent(NULL, I2S_PAGE_SIZE, ptri2s_config->pMMAPBufPtr[i], i2s_mmap_addr[i]);
					ptri2s_config->pMMAPBufPtr[i] = NULL;	
			}
	}
	/* free buffer */
	if(ptri2s_config->pPage0TxBuf8ptr)
	{
			pci_free_consistent(NULL, I2S_PAGE_SIZE*2, ptri2s_config->pPage0TxBuf8ptr, i2s_txdma_addr);
			ptri2s_config->pPage0TxBuf8ptr = NULL;
	}
	
	kfree(ptri2s_config);
			
#ifdef I2S_STATISTIC
	kfree(pi2s_status);/*释放设备结构体内存*/
#endif

	return 0;
}

static int i2s_mmap(struct file *filp, struct vm_area_struct *vma)
{
	static int mmap_index = 0;
  int nRet;
	unsigned long size = vma->vm_end-vma->vm_start;
	
	if((pi2s_config->pMMAPBufPtr[0]==NULL)&&(mmap_index!=0))
	{
			mmap_index = 0;
	}	
	if(pi2s_config->pMMAPBufPtr[mmap_index]!=NULL)
	{
			goto EXIT;
	}
	//分配一片DMA缓冲区
	pi2s_config->pMMAPBufPtr[mmap_index] = (u8*)pci_alloc_consistent(NULL, size , &i2s_mmap_addr[mmap_index]);
	
	if( pi2s_config->pMMAPBufPtr[mmap_index] == NULL ) 
	{
			return -1;
	}
EXIT:	 	
	memset(pi2s_config->pMMAPBufPtr[mmap_index], 0, size);//置0初始化
	/**
	*@直接映射分配，将这段内存直接映射到vm上。调用 remap_pfn_range 一次完成建立页表
	*/	
	nRet = remap_pfn_range(vma, vma->vm_start, virt_to_phys((void *)pi2s_config->pMMAPBufPtr[mmap_index]) >> PAGE_SHIFT,  size, vma->vm_page_prot);
	
	if( nRet != 0 )
	{
			return -EIO;
	}

	mmap_index++;
#if defined(CONFIG_I2S_TXRX)
	if(mmap_index>=MAX_I2S_PAGE*2)
#else	
	if(mmap_index>=MAX_I2S_PAGE)
#endif	
		mmap_index = 0;
	return 0;
}

int i2s_reset_tx_config(i2s_config_type* ptri2s_config)
{
	ptri2s_config->bTxDMAEnable = 0;
	ptri2s_config->tx_isr_cnt = 0;
	ptri2s_config->tx_w_idx = 0;
	ptri2s_config->tx_r_idx = 0;	
	pi2s_status->txbuffer_unrun = 0;
	pi2s_status->txbuffer_ovrun = 0;
	pi2s_status->txdmafault = 0;
	pi2s_status->txovrun = 0;
	pi2s_status->txunrun = 0;
	pi2s_status->txthres = 0;
	pi2s_status->txbuffer_len = 0;	
	return 0;
}

int i2s_clock_enable(i2s_config_type* ptri2s_config)
{
	unsigned long data;	
	/* enable internal MCLK */
#if defined(CONFIG_I2S_IN_MCLK)
	data = i2s_inw(RALINK_SYSCTL_BASE+0x2c);	
#if defined(CONFIG_RALINK_RT5350)
	data &= ~(0x0F<<8);//8-11四位清0
	data |= (0x3<<8);	//P26 ,REFCLK0_RATE=12M; MCS1_AS_REFCLK0=1,Reference clock 0 output
#else
	#error "This SoC do not provide MCLK to audio codec\n");	
#endif
	i2s_outw(RALINK_SYSCTL_BASE+0x2c, data);	
#endif		
	
	/* set share pins to i2s/gpio mode and i2c mode */
	data = i2s_inw(RALINK_SYSCTL_BASE+0x60); 
	data &= 0xFFFFFFE2;//00010; 2-4三位清0
	data |= 0x00000019;//11000；3'b110设为i2s模式，gpio11-14
	i2s_outw(RALINK_SYSCTL_BASE+0x60, data);
		
	if(ptri2s_config->slave_en==0)
	{
		data = 78;
		i2s_outw(I2S_DIVINT_CFG, data);
		data = 64;
		data |= REGBIT(1, I2S_CLKDIV_EN); 
		i2s_outw(I2S_DIVCOMP_CFG, data);

	
		#if defined(CONFIG_I2S_WS_EDGE)
			data = i2s_inw(I2S_I2SCFG);
			data |= REGBIT(0x1, I2S_WS_INV);
			i2s_outw(I2S_I2SCFG, data);
		#endif
		data = i2s_inw(I2S_I2SCFG);
		data &= ~REGBIT(0x1, I2S_SLAVE_MODE);
		i2s_outw(I2S_I2SCFG, data);
	  audiohw_set_frequency(data|0x01);

	}
	data = i2s_inw(I2S_I2SCFG);
	data |= REGBIT(0x1, I2S_EN);
	i2s_outw(I2S_I2SCFG, data);	
	return 0;
		
}	

int i2s_clock_disable(i2s_config_type* ptri2s_config)
{
	unsigned long data;
	/* disable internal MCLK */
#if defined(CONFIG_I2S_IN_MCLK)	
	data = i2s_inw(RALINK_SYSCTL_BASE+0x2c);
	data &= ~(0x03<<8);
	i2s_outw(RALINK_SYSCTL_BASE+0x2c, data);	
#endif
	return 0;
}	


int i2s_tx_config(i2s_config_type* ptri2s_config)
{
	unsigned long data;
	/* set I2S_I2SCFG */
	data = i2s_inw(I2S_I2SCFG);
	data &= 0xFFFFFF81;//10000001
	data |= REGBIT(ptri2s_config->tx_ff_thres, I2S_TX_FF_THRES);//4
	data |= REGBIT(ptri2s_config->tx_ch_swap, I2S_TX_CH_SWAP);//0
	data &= ~REGBIT(1, I2S_TX_CH0_OFF);//????????????	
	data &= ~REGBIT(1, I2S_TX_CH1_OFF);////???????//
	i2s_outw(I2S_I2SCFG, data);
	
#if defined(CONFIG_I2S_EXTENDCFG)	
	/* set I2S_I2SCFG1 */
	data = i2s_inw(I2S_I2SCFG1);
	data |= REGBIT(ptri2s_config->lbk, I2S_LBK_EN);// Normal mode
	data |= REGBIT(ptri2s_config->extlbk, I2S_EXT_LBK_EN);//Normal mode 
	data &= 0xFFFFFFFC;//处理后两位
	data |= REGBIT(ptri2s_config->fmt, I2S_DATA_FMT);////????????????????????
	i2s_outw(I2S_I2SCFG1, data);
#endif
	
	return 0;
}	



/* Turn On Tx DMA and INT */
int i2s_tx_enable(i2s_config_type* ptri2s_config)
{
	unsigned long data;
	data = i2s_inw(I2S_INT_EN);
	data |= REGBIT(0x1, I2S_TX_INT3_EN);
	data |= REGBIT(0x1, I2S_TX_INT2_EN);
	data |= REGBIT(0x1, I2S_TX_INT1_EN);
	i2s_outw(I2S_INT_EN, data);
	data = i2s_inw(I2S_I2SCFG);	
	data |= REGBIT(0x1, I2S_TX_EN);
	ptri2s_config->bTxDMAEnable = 1;
	data |= REGBIT(0x1, I2S_DMA_EN);
	i2s_outw(I2S_I2SCFG, data);	
	data = i2s_inw(I2S_I2SCFG);
	data |= REGBIT(0x1, I2S_EN);
	i2s_outw(I2S_I2SCFG, data);
	return I2S_OK;
}


/* Turn Off Tx DMA and INT */
int i2s_tx_disable(i2s_config_type* ptri2s_config)
{
	unsigned long data;	
	data = i2s_inw(I2S_INT_EN);
	data &= ~REGBIT(0x1, I2S_TX_INT3_EN);
	data &= ~REGBIT(0x1, I2S_TX_INT2_EN);
	data &= ~REGBIT(0x1, I2S_TX_INT1_EN);
	i2s_outw(I2S_INT_EN, data);
	
	data = i2s_inw(I2S_I2SCFG);
#if defined(CONFIG_I2S_TXRX)	
	data &= ~REGBIT(0x1, I2S_TX_EN);
	data &= ~REGBIT(0x1, I2S_EN);
#else
	data &= ~REGBIT(0x1, I2S_EN);
#endif	
	if(ptri2s_config->bRxDMAEnable==0)
	{
			ptri2s_config->bTxDMAEnable = 0;
			data &= ~REGBIT(0x1, I2S_DMA_EN);
	}
	i2s_outw(I2S_I2SCFG, data);
	
	return I2S_OK;
}

	
void i2s_dma_tx_handler(u32 dma_ch)
{
	u32 i2s_status;
	
	i2s_status=i2s_inw(I2S_INT_STATUS);
	pi2s_config->tx_isr_cnt++;
	
	if(pi2s_config->bTxDMAEnable==0)
	{
			return;
	}

	
	if(pi2s_config->tx_r_idx==pi2s_config->tx_w_idx)
	{
		/* Buffer Empty */
		pi2s_status->txbuffer_unrun++;	
		if(dma_ch==GDMA_I2S_TX0)
		{
			GdmaI2sTx((u32)pi2s_config->pPage0TxBuf8ptr, I2S_TX_FIFO_WREG, 0, I2S_PAGE_SIZE, i2s_dma_tx_handler, i2s_unmask_handler);
		}
		else
		{
			GdmaI2sTx((u32)pi2s_config->pPage1TxBuf8ptr, I2S_TX_FIFO_WREG, 1, I2S_PAGE_SIZE, i2s_dma_tx_handler, i2s_unmask_handler);
		}
		
		goto EXIT;	
	}
	
	if(pi2s_config->pMMAPTxBufPtr[pi2s_config->tx_r_idx]==NULL)
	{
		if(dma_ch==GDMA_I2S_TX0)
		{	
			GdmaI2sTx((u32)pi2s_config->pPage0TxBuf8ptr, I2S_TX_FIFO_WREG, 0, I2S_PAGE_SIZE, i2s_dma_tx_handler, i2s_unmask_handler);
		}	
		else
		{	
			GdmaI2sTx((u32)pi2s_config->pPage1TxBuf8ptr, I2S_TX_FIFO_WREG, 1, I2S_PAGE_SIZE, i2s_dma_tx_handler, i2s_unmask_handler);
		}	
		goto EXIT;	
	}

	pi2s_status->txbuffer_len--;
	if(dma_ch==GDMA_I2S_TX0)
	{	
		memcpy(pi2s_config->pPage0TxBuf8ptr,  pi2s_config->pMMAPTxBufPtr[pi2s_config->tx_r_idx], I2S_PAGE_SIZE);			
		GdmaI2sTx((u32)(pi2s_config->pPage0TxBuf8ptr), I2S_TX_FIFO_WREG, 0, I2S_PAGE_SIZE, i2s_dma_tx_handler, i2s_unmask_handler);
		pi2s_config->dmach = GDMA_I2S_TX0;
		pi2s_config->tx_r_idx = (pi2s_config->tx_r_idx+1)%MAX_I2S_PAGE;
	}
	else
	{
		memcpy(pi2s_config->pPage1TxBuf8ptr,  pi2s_config->pMMAPTxBufPtr[pi2s_config->tx_r_idx], I2S_PAGE_SIZE);
		GdmaI2sTx((u32)(pi2s_config->pPage1TxBuf8ptr), I2S_TX_FIFO_WREG, 1, I2S_PAGE_SIZE, i2s_dma_tx_handler, i2s_unmask_handler);
		pi2s_config->dmach = GDMA_I2S_TX1;
		pi2s_config->tx_r_idx = (pi2s_config->tx_r_idx+1)%MAX_I2S_PAGE;		
	}
EXIT:	
	wake_up_interruptible(&(pi2s_config->i2s_tx_qh));
	return;
}


irqreturn_t i2s_irq_isr(int irq, void *irqaction)
{
	u32 i2s_status;

	if((pi2s_config->tx_isr_cnt>0)||(pi2s_config->rx_isr_cnt>0))
	{
		i2s_status=i2s_inw(I2S_INT_STATUS);
		MSG("511i2s_irq_isr [0x%08X]\n",i2s_status);
	}
	else
		return IRQ_HANDLED;
		
	if(i2s_status&REGBIT(1, I2S_TX_DMA_FAULT))
	{
		pi2s_status->txdmafault++;
	}
	if(i2s_status&REGBIT(1, I2S_TX_OVRUN))
	{
		pi2s_status->txovrun++;
	}
	if(i2s_status&REGBIT(1, I2S_TX_UNRUN))
	{
		pi2s_status->txunrun++;
	}
	if(i2s_status&REGBIT(1, I2S_TX_THRES))
	{
		pi2s_status->txthres++;
	}
	i2s_outw(I2S_INT_STATUS, 0xFFFFFFFF);
	return IRQ_HANDLED;
}


void i2s_unmask_handler(u32 dma_ch)
{
	if((dma_ch==GDMA_I2S_TX0)&&(pi2s_config->bTxDMAEnable))
	{
		GdmaI2sTx((u32)pi2s_config->pPage0TxBuf8ptr, I2S_FIFO_WREG, 0, I2S_PAGE_SIZE, i2s_dma_tx_handler, i2s_unmask_handler);
		GdmaUnMaskChannel(GDMA_I2S_TX0);
	}
	if((dma_ch==GDMA_I2S_TX1)&&(pi2s_config->bTxDMAEnable))
	{
		GdmaI2sTx((u32)pi2s_config->pPage1TxBuf8ptr, I2S_FIFO_WREG, 1, I2S_PAGE_SIZE, i2s_dma_tx_handler, i2s_unmask_handler);
		GdmaUnMaskChannel(GDMA_I2S_TX1);
	}
}

int i2s_ioctl (struct inode *inode, struct file *filp, unsigned int cmd, unsigned long arg)
{
	int i ;
	unsigned long flags, data;
	i2s_config_type* ptri2s_config;
	
	ptri2s_config = filp->private_data;
	switch (cmd) {
	case I2S_SRATE:
		spin_lock_irqsave(&ptri2s_config->lock, flags);
		if((arg>MAX_SRATE_HZ)||(arg<MIN_SRATE_HZ))
		{
				MSG("audio sampling rate %u should be %d ~ %d Hz\n", (u32)arg, MIN_SRATE_HZ, MAX_SRATE_HZ);
				break;
		}	
		ptri2s_config->srate = arg;
		MSG("audio sampling rate to %d Hz\n", ptri2s_config->srate);
		spin_unlock_irqrestore(&ptri2s_config->lock, flags);
		break;

	case I2S_TX_ENABLE:
		spin_lock_irqsave(&ptri2s_config->lock, flags);
		MSG("I2S_TXENABLE\n");
		ptri2s_config->pPage0TxBuf8ptr = (u8*)pci_alloc_consistent(NULL, I2S_PAGE_SIZE*2 , &i2s_txdma_addr);
		if(ptri2s_config->pPage0TxBuf8ptr==NULL)
		{
				MSG("Allocate Tx Page Buffer Failed\n");
				return -1;
		}
		ptri2s_config->pPage1TxBuf8ptr = ptri2s_config->pPage0TxBuf8ptr + I2S_PAGE_SIZE;
		for( i = 0 ; i < MAX_I2S_PAGE ; i ++ )
		{
			if(ptri2s_config->pMMAPTxBufPtr[i]==NULL)
			{
					ptri2s_config->pMMAPTxBufPtr[i] = kmalloc(I2S_PAGE_SIZE, GFP_KERNEL);
			}
		}
    MSG("GdmaI2sTx\n");
		GdmaI2sTx((u32)ptri2s_config->pPage0TxBuf8ptr, I2S_FIFO_WREG, 0, I2S_PAGE_SIZE, i2s_dma_tx_handler, i2s_unmask_handler);
		GdmaI2sTx((u32)ptri2s_config->pPage1TxBuf8ptr, I2S_FIFO_WREG, 1, I2S_PAGE_SIZE, i2s_dma_tx_handler, i2s_unmask_handler);
		i2s_reset_tx_config(ptri2s_config);
		ptri2s_config->bTxDMAEnable = 1;
		i2s_tx_config(ptri2s_config);
		
		if(ptri2s_config->bRxDMAEnable==0)
		{
				i2s_clock_enable(ptri2s_config);
		}
		i2s_tx_enable(ptri2s_config);
		GdmaUnMaskChannel(GDMA_I2S_TX0);
		data = i2s_inw(RALINK_REG_INTENA);
		data |=0x0400;
	  i2s_outw(RALINK_REG_INTENA, data);
		spin_unlock_irqrestore(&ptri2s_config->lock, flags);
		break;
	case I2S_TX_DISABLE:
		spin_lock_irqsave(&ptri2s_config->lock, flags);
		MSG("I2S_TXDISABLE\n");
		i2s_reset_tx_config(ptri2s_config);
		if(ptri2s_config->bRxDMAEnable==0)
		{
				i2s_clock_disable(ptri2s_config);
		}
		i2s_tx_disable(ptri2s_config);
		if(ptri2s_config->bRxDMAEnable==0)
		{
				data = i2s_inw(RALINK_REG_INTENA);
				data &= 0xFFFFFBFF;
		    i2s_outw(RALINK_REG_INTENA, data);
		}
		
		for( i = 0 ; i < MAX_I2S_PAGE ; i ++ )
		{
			if(ptri2s_config->pMMAPTxBufPtr[i] != NULL)
			{
				kfree(ptri2s_config->pMMAPTxBufPtr[i]);		
			  ptri2s_config->pMMAPTxBufPtr[i] = NULL;
			}
		}
		pci_free_consistent(NULL, I2S_PAGE_SIZE*2, ptri2s_config->pPage0TxBuf8ptr, i2s_txdma_addr);
		ptri2s_config->pPage0TxBuf8ptr = NULL;
		spin_unlock_irqrestore(&ptri2s_config->lock, flags);
		break;
	
	case I2S_PUT_AUDIO:	
		do{
			spin_lock_irqsave(&ptri2s_config->lock, flags);
			if(((ptri2s_config->tx_w_idx+4)%MAX_I2S_PAGE)!=ptri2s_config->tx_r_idx)
			{	
				ptri2s_config->tx_w_idx = (ptri2s_config->tx_w_idx+1)%MAX_I2S_PAGE;		
				copy_from_user(ptri2s_config->pMMAPTxBufPtr[ptri2s_config->tx_w_idx], (char*)arg, I2S_PAGE_SIZE);
				pi2s_status->txbuffer_len++;
				spin_unlock_irqrestore(&ptri2s_config->lock, flags);	
				break;
			}
			else
			{
				/* Buffer Full */
				pi2s_status->txbuffer_ovrun++;
				spin_unlock_irqrestore(&ptri2s_config->lock, flags);
				interruptible_sleep_on(&(ptri2s_config->i2s_tx_qh));				
			}
		}while(1);
		break;					
	default :
		MSG("i2s_ioctl: command  will be set later\n");
	}
	return 0;
}

module_init(i2s_mod_init);
module_exit(i2s_mod_exit);

MODULE_DESCRIPTION("Ralink SoC I2S Controller Module");
MODULE_AUTHOR("Qwert Chin <qwert.chin@ralinktech.com.tw>");
MODULE_SUPPORTED_DEVICE("I2S");
MODULE_VERSION(I2S_MOD_VERSION);
MODULE_LICENSE("GPL");

#if LINUX_VERSION_CODE <= KERNEL_VERSION(2,6,12)
MODULE_PARM (i2sdrv_major, "i");
#else
module_param (i2sdrv_major, int, 0);
#endif
