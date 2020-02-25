/** @file [VideoAlarm.c]
 *  @brief 视频报警线程的主要功能函数C文件
 *	@author: Summer.Wang
 *	@data: 2013-9-24
 *	@modify record: 即日创建了这套文件注释系统
 */
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <stdarg.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <semaphore.h>
#include <time.h>
#include <fcntl.h>
#include <termios.h>
#include <jpeglib.h>
#include "common/thread.h"
#include "common/common.h"
#include "common/msg_queue.h"
#include "common/common_config.h"
#include "common/logfile.h"
#include "common/utils.h"
#include "common/logserver.h"
#include "VideoAlarm.h"
#include "VideoAlarmAdapter.h"
#include "Video/video.h"
#include "HTTPServer/do_https_m3s.h"

struct VideoAlarm_t *VideoAlarm_list;///< 该文件中应用到的多个函数间传递的变量封装成此结构体型全局变量
VA_BITMAP bitmapBack,bitmapFore,bitmapDetect;///< 图像处理中涉及到的三个存储位图图像的全局变量

/*-----------内部函数声明-----------*/
void *VideoAlarm_Thread(void *arg);
void VideoAlarm_ThreadSync(void);
int VideoAlarm_Module_Init(msg_container* self);
void VideoAlarm_Variable_Init(struct VideoAlarm_t *self);
void VideoAlarm_Msg_Receive(struct VideoAlarm_t* self,VA_SRCIMAGE* srcImageBack,VA_SRCIMAGE* srcImageFore);
void VideoAlarm_Msg_Process(struct VideoAlarm_t* self,VA_SRCIMAGE* srcImageBack,VA_SRCIMAGE* srcImageFore);
void VideoAlarm_Msg_Send(msg_container *self,int command);
void VideoAlarm_ChangeBackImage(struct VideoAlarm_t* self);
void VideoAlarm_NoAlarmSection(struct VideoAlarm_t* self);
int VideoAlarm_threshold(VA_BITMAP *bitmap);
int VideoAlarm_DetectFrameFail(VA_SRCIMAGE *srcImage,VA_BITMAP *bitmap,struct VideoAlarm_t* self);
int VideoAlarm_getBackFrame(VA_SRCIMAGE *srcImage,VA_BITMAP *bitmapBack);
int VideoAlarm_getForeFrame(VA_SRCIMAGE *srcImage,VA_BITMAP *bitmapFore);
int VideoAlarm_comp(VA_BITMAP *bitmapBack, VA_BITMAP *bitmapFore,struct VideoAlarm_t* self);
char VideoAlarm_ImageCmp(VA_BITMAP *bitmapBack, VA_BITMAP *bitmapFore,struct VideoAlarm_t* self);
void VideoAlarm_BackImageProcess(VA_SRCIMAGE *srcImageBack,VA_BITMAP *bitmapBack,struct VideoAlarm_t* self);
void VideoAlarm_ForeImageCompare(VA_SRCIMAGE *srcImageFore,VA_BITMAP *bitmapBack,VA_BITMAP *bitmapFore,struct VideoAlarm_t* self);
void VideoAlarm_StorePicToFile(char *filename,char *datapoint,int len,struct VideoAlarm_t* self);
void VideoAlarm_TriggerAlarm(VA_SRCIMAGE *srcImageBack,VA_SRCIMAGE *srcImageFore,struct VideoAlarm_t* self);
void VideoAlarm_UploadPic(VA_SRCIMAGE *srcImage,struct VideoAlarm_t* self);

void VideoAlarm_Log_S(const char *fmt, ...)
{
#define BUF_SIZE 1024
	char buf[BUF_SIZE];
	int n;
	va_list ap;

	memset(buf, 0, sizeof(buf));
	strcpy(buf, "Videoalarm:");
	n = strlen(buf);
	va_start(ap, fmt);
	vsnprintf(&buf[n], 1024, fmt, ap);
	n = strlen(buf);
	if(n > BUF_SIZE-2){
		fprintf(stderr, "%s: log big\n", __FUNCTION__);
		return;
	}
	buf[n] = '\n';
	n++;
	buf[n] = '\0';
	n++;
	va_end(ap);
	log_s(buf, n);
}

/**
 * @brief 线程创建函数
 * @author Summer.Wang
 * @return <失败返回-1><成功返回线程ID>
 * @note 线程控制函数
 */
int VideoAlarm_Thread_Creat() {
	int result;
	result = thread_start(VideoAlarm_Thread);
	return result;
}

/**
 * @brief 线程停止函数
 * @author Summer.Wang
 * @return 0
 * @note 线程控制函数
 */
int VideoAlarm_Thread_Stop() {
	return 0;
}

/**
 * @brief 线程同步函数
 * @author Summer.Wang
 * @return Null
 * @note 线程控制函数
 * @param[out] <rVideoAlarm><置位全局变量>
 */
void VideoAlarm_ThreadSync(){
	rVideoAlarm=1;
	while(rChildProcess == 0){
		thread_usleep(0);
	}
	thread_usleep(0);
}

/**
 * @brief 消息队列初始化函数
 * @author Summer.Wang
 * @return <成功返回0><失败返回-1>
 * @param[out] <self><控制模块属性>
 * @note 获取两个消息队列的ID
 */
int VideoAlarm_Module_Init(msg_container* self)
{
	self->lmsgid = msg_queue_get((key_t)CHILD_THREAD_MSG_KEY);
	if( self->lmsgid == -1) {
		return -1;
	}
	self->rmsgid = msg_queue_get((key_t)CHILD_PROCESS_MSG_KEY);
	if( self->rmsgid == -1) {
		return -1;
	}
	return 0;
}

/**
 * @brief 线程变量初始化函数
 * @author Summer.Wang
 * @return Null
 * @param[out] <self><控制模块属性>
 * @note 将包裹在全局结构体内的关键变量进行初始化，读出flash中的报警开关和级别进行转换
 */
void VideoAlarm_Variable_Init(struct VideoAlarm_t *self)
{
	int length;

	self->alarmswitch = 0;
	get_config_item_value(CONFIG_ALARM_MOTION,&self->alarmswitch,&length);
	if(*((char*)(&self->alarmswitch)) == 'n')	self->alarmswitch = 0;//读取flash为n或2时关闭报警
	else if(*((char*)(&self->alarmswitch)) == 'y')	self->alarmswitch = 1;//读取flash为y或1时开启报警
	else if(*((char*)(&self->alarmswitch)) == '2')	self->alarmswitch = 0;
	else if(*((char*)(&self->alarmswitch)) == '1')	self->alarmswitch = 1;
	else self->alarmswitch = 0;//其他默认为关闭

#if 0
	self->alarmswitch -= '0';
	if(self->alarmswitch != 1)	self->alarmswitch = 0;//不为1的其他值都默认为关
//	if(self->alarmswitch == 2)	self->alarmswitch = 0;
#endif

	self->MotionSense = 0;
	get_config_item_value(CONFIG_ALARM_MOTION_SENSITIVITY,&self->MotionSense,&length);
	self->MotionSense -= '0';
	if(self->MotionSense > 9 || self->MotionSense < 1)	self->MotionSense = 5;//报警级别不为1－9则默认为9

	self->fetch = 0;
	self->old_fetch = 0;
	self->holdback = 0;
	self->holdfore = 0;
	self->noalarm = 0;
	self->alarm = 0;
	self->getback = 0;
	self->getfore = 0;
	self->store = 0;
	self->alarm_type = 0;
	self->autio_start = 0;
	self->count_back = 0;
	self->count_noalarm = 0;
	self->count_store = 0;
	self->count_fetch = 0;
	self->old_alarmswitch = 0;
	self->Motormove = 0;
	self->framefail = 0;
	self->count_framfail = 0;
	self->count_noalarm_bug = 0;
	self->noise = 64;
	self->light = 90;
}

/**
 * @brief 消息队列接受函数
 * @author Summer.Wang
 * @return Null
 * @param[in|out] <self><控制模块属性>
 * @note 内部嵌套着消息处理函数
 */
void VideoAlarm_Msg_Receive(struct VideoAlarm_t* self,VA_SRCIMAGE* srcImageBack,VA_SRCIMAGE* srcImageFore)
{
	msg_container* msg = (msg_container*)self;
	if(msg_queue_rcv(msg->lmsgid, msg->msgdata_rcv, MAX_MSG_LEN, VIDEO_ALARM_MSG_TYPE) != -1)
	{
		VideoAlarm_Msg_Process(self,srcImageBack,srcImageFore);
	}
}

/**
 * @brief 消息队列发送函数
 * @author Summer.Wang
 * @return Null
 * @param[in] <self><控制模块属性>
 * @param[in] <command><被发送消息的command值>
 * @note 用于线程发送给子进程的消息，统一封装为长度为零的消息触发子进程进入消息适配器模块进行数据处理和发送
 */
void VideoAlarm_Msg_Send(msg_container *self,int command)
{
	MsgData *msgsnd = (MsgData *)self->msgdata_snd;
	msgsnd->len = 0;
	msgsnd->type = VIDEO_ALARM_MSG_TYPE;
	msgsnd->cmd = command;
	if(msg_queue_snd(self->rmsgid, self->msgdata_snd, sizeof(struct MsgData)-sizeof(long int)) == -1)
	{
		return;
	}
}

/**
 * @brief 消息队列接受数据处理函数
 * @author Summer.Wang
 * @return Null
 * @param[in|out] <self><控制模块属性>
 * @param[out] <srcImageBack><用于释放帧>
 * @param[out] <srcImageFore><用于释放帧>
 * @note 根据接受到的消息进行各式各样的处理啦~
 */
void VideoAlarm_Msg_Process(struct VideoAlarm_t* self,VA_SRCIMAGE* srcImageBack,VA_SRCIMAGE* srcImageFore)
{
	msg_container *msg = (msg_container*)self;
	VA_MsgData *msgrcv = (VA_MsgData *)msg->msgdata_rcv;

	switch(msgrcv->cmd)
	{
		case MSG_VA_T_START:{
			self->count_fetch = 50;
			break;
		}
		case MSG_VA_T_STOP:{
			self->fetch = 0;
			if(self->holdback == 1){
				ReleaseNewestVideoFrame(srcImageBack->iIndex);
				self->holdback = 0;
			}
			if(self->holdfore == 1){
				ReleaseNewestVideoFrame(srcImageFore->iIndex);
				self->holdfore = 0;
			}
			VideoAlarm_Msg_Send(msg,MSG_VA_P_REPLY);
			break;
		}
		case MSG_VA_T_CONTROL:{
			self->alarmswitch = *(int*)(msgrcv->data);
            if(self->alarmswitch == 2)	self->alarmswitch=0;
			self->MotionSense = *(int*)(msgrcv->data + sizeof(int));
			VideoAlarm_Msg_Send(msg,MSG_VA_P_CONTROL);
			break;
		}
		case MSG_VA_T_ALRAM:{
			if(self->noalarm == 0){
				if(*(int*)(msgrcv->data)==1){
					self->alarm = 1;//触发报警
					self->alarm_type = 1;//音频报警
					self->noalarm = 1;/*2013-8-27需求：音频报警后视频30秒内不能报警*/
					self->count_noalarm = 0;//初始化报警模块
					VideoAlarm_Msg_Send(msg,MSG_VA_P_AUDIO_START);
					self->autio_start = 1;
				}//音频报警开始
#ifdef _IMG_TYPE_BELL
				else {
					self->alarm = 1;//触发报警
					self->alarm_type = 2;//音频报警
//					self->noalarm = 0;/*2013-8-27需求：音频报警后视频30秒内不能报警*/
					self->count_noalarm = 0;//初始化报警模块
//					VideoAlarm_Msg_Send(msg,MSG_VA_P_AUDIO_START);
//					self->autio_start = 1;
				}
#endif
			}
			if(*(int*)(msgrcv->data)==2){
				if(self->autio_start == 1){
					VideoAlarm_Msg_Send(msg,MSG_VA_P_AUDIO_STOP);
					self->autio_start = 0;
				}
			}//音频报警结束
			break;
		}
		case MSG_VA_T_MOTOR:{
			if(*(int*)(msgrcv->data)== 1){
				self->getback = 1;//电机开始转动后更换背景
				self->Motormove = 1;
			}
			else if(*(int*)(msgrcv->data)== 0){
				self->getback = 1;//电机结束转动后更换背景，防止停止转动后立即取前景触发报警，更正电机转动误报警
				self->Motormove = 0;
			}
			break;
		}
		case MSG_VA_T_TEST:{
			VideoAlarm_Msg_Send(msg,MSG_VA_P_ALIVE);
			break;
		}
		default:
			break;
	}
}

/**
 * @brief 线程循环计数模块
 * @author Summer.Wang
 * @return Null
 * @param[in|out] <self><控制模块属性>
 * @note 封装了几种更换背景帧的方式，封装了利用线程循环计数和置位的变量改变
 */
void VideoAlarm_ChangeBackImage(struct VideoAlarm_t* self)
{
	if(!self->noalarm)//没有关闭报警才数数
	{
		self->count_back++;
	}
	if(self->count_back == 30)
	{
		self->getback = 1;
		self->count_back = 0;
	}

	if(self->old_fetch != self->fetch)/*在取帧条件变化的情况下需要更换背景*/
	{
		self->getback = 1;
		self->old_fetch = self->fetch;
	}

	if(self->old_alarmswitch != self->alarmswitch)/*在报警开关条件变化的情况下需要更换背景*/
	{
		self->getback = 1;
		self->old_alarmswitch = self->alarmswitch;
	}

	if(self->count_fetch){//修正刚开始取视频的黑屏现象
		self->count_fetch--;
		if(self->count_fetch == 0){
			self->fetch = 1;
		}
	}

	self->getfore = 1; //每次循环取一个前景
}

/**
 * @brief 30秒不报警计时模块
 * @author Summer.Wang
 * @return Null
 * @param[in|out] <self><控制模块属性>
 * @note 取时间戳并比对
 */
void VideoAlarm_NoAlarmSection(struct VideoAlarm_t* self)
{
	time_t ts;
	int result;
	if(self->noalarm){
#if 1
		if(self->count_noalarm == 0){
			time(&self->count_noalarm);
		}
		time(&ts);
		result = ts - self->count_noalarm;
		if(result >= 30){
			self->count_noalarm = 0;
			self->noalarm = 0;
		}
#endif
#if 0
		self->count_noalarm++;
		if(self->count_noalarm == 200){
			self->count_noalarm = 0;
			self->noalarm = 0;
		}
#endif
	}
}

/**
 * @brief 图像二值化算法
 * @author Summer.Wang
 * @return <失败返回-1><成功返回二值化算法后返回的阈值>
 * @param[in] <bitmap><待处理的位图图像结构>
 * @note otsu算法
 */
int VideoAlarm_threshold(VA_BITMAP *bitmap)
{
	int iCount=0,iTemp=0;
	int iSum=bitmap->height*bitmap->width;
	int iSumBack=0,iSumFore=0;// back,fore image pixel sum；
   	int thresholdValue=0;//threshold
	int iHist[256]={0};//histogram array；
	unsigned char uPixel=0;//save the value of pixel；
	float dAverage1=0,dAverage2=0;//average value of the back and foreimage;
	float dPixelsum=0.0,dBpixelsum=0.0;// all Gray orders of magnitude;back Gray orders of magnitude
	float dVmax=-1.0,dVar=0.0;//dVmax:Otsu method;dVar:interclass variance

	if(bitmap==NULL)
	{
		return -1;
	}
	for(iCount=0;iCount<iSum;iCount++)//Stat-Histogram;
	{//得到灰度直方图并存储在iHist中
		uPixel=*(bitmap->buf+iCount);
		iHist[uPixel]++; //the count which gray value is uPixel
	}
	for(iCount=0;iCount<=255;iCount++)//count Total grayscale;
	{//总体灰度值
		dPixelsum+=iCount*iHist[iCount];
	}
	for(iCount=0;iCount<=255;iCount++)//count Otsu
	{//遍历所有灰度值，加入了255
		iSumBack+=iHist[iCount];//将从0开始到某像素值求和作为背景像素总数
//		if(!iSumBack)
//		{
//			continue;
//		}
		iSumFore=iSum-iSumBack;//作差就可以得到从某像素到255的前景像素总和
		if(iSumFore==0)
		{
			break;
		}
		dBpixelsum+=iCount*iHist[iCount];//Gray orders of magnitude of backimage
		//背景平均灰度
		dAverage1=dBpixelsum/iSumBack;//average gradation of backimage
		//前景平均灰度
		dAverage2=(dPixelsum-dBpixelsum)/iSumFore;//average gradation of foreimage
		/* Ostu formula*/
		//计算类间方差
//		dVar = iSumBack*(dAverage1-dPixelsum)*(dAverage1-dPixelsum) + iSumFore*(dAverage2-dPixelsum)*(dAverage2-dPixelsum);
		dVar=iSumBack*iSumFore*(dAverage1-dAverage2)*(dAverage1-dAverage2);
		if(dVar>dVmax)
		{
			dVmax=dVar;
			iTemp=iCount;
		}
	}
	if(70>=iTemp)
	{
		thresholdValue=137;
	}
	else if(70<iTemp&&iTemp<=110)
	{
		thresholdValue=40;
	}
	else if(110<iTemp&&iTemp<=200)
	{
		thresholdValue=iTemp;//65;
	}
	else if(200<iTemp)
	{
		thresholdValue=160;
	}
	else
	{
		thresholdValue=iTemp;
	}
//    thresholdValue = iTemp;
	for(iCount=0;iCount<iSum;iCount++)/*图像分割*/
	{
		if(*(bitmap->buf+iCount)>=thresholdValue)
		{
			*(bitmap->buf+iCount)=255;
		}
		else
		{
			*(bitmap->buf+iCount)=0;
		}
	}
	return thresholdValue;
}

/**
 * @brief 检测花屏和白屏的处理模块
 * @author Summer.Wang
 * @return <失败返回-1><成功返回0>
 * @param[in] <self><控制模块属性>
 * @param[in] <srcImage><待处理的jpeg图像结构>
 * @param[in] <bitmap><处理后的图像位图结构>
 * @note 功能模块，得到该帧图像是否有花屏或白屏现象的结论
 */
int VideoAlarm_DetectFrameFail(VA_SRCIMAGE *srcImage,VA_BITMAP *bitmap,struct VideoAlarm_t* self)
{
	if(self->fetch==1)
	{
		/*因为花屏可能触发报警，为了及时检测，去掉检测花屏的时间间隔*/
		//注意这个全局变量已经在别的地方应用了
//		self->count_framfail++;
//		if(self->count_framfail == 5)//每近半秒检测一次花屏
//		{
//			self->count_framfail = 0;
		if(self->framefail == 0)//检测到花屏后近5秒才能再次检测
		{
			int result = GetNewestVideoFrame(&srcImage->srcdata, &srcImage->iLen, &srcImage->iIndex);
			if(result == -1){
				return -1;
			}
			else if(srcImage->srcdata==NULL || srcImage->iIndex<0 || srcImage->iLen==0){
				return -1;
			}

			if(-1==ReadJpegBuf(srcImage->srcdata, srcImage->iLen, bitmap))/*解码，灰度处理*/
			{
				msg_container *msg = (msg_container*)self;
				VideoAlarm_Msg_Send(msg,MSG_VA_P_FRAME);
				self->framefail = 17;//50;//用于花屏多久后不再检测花屏
				self->count_framfail = 5;//15;//用于花屏后多久不再报警
			}
			ReleaseNewestVideoFrame(srcImage->iIndex);
		}
		else
		{
			self->framefail--;
		}

		if(self->count_framfail != 0)
		{
			self->getfore = 0;//如果检测到花屏，则几个循环内不取视频
			self->getback = 0;
			self->count_framfail--;
		}

//		}
	}
	return 0;
}

/**
 * @brief 处理背景帧的解码、二值化操作
 * @author Summer.Wang
 * @return <失败返回-1><成功返回0>
 * @param[in] <srcImage><待处理的jpeg图像结构>
 * @param[in] <bitmapBack><处理后的图像位图结构>
 */
int VideoAlarm_getBackFrame(VA_SRCIMAGE *srcImage,VA_BITMAP *bitmapBack)
{
	if(srcImage->srcdata==NULL||bitmapBack->buf==NULL)
	{
		return -1;
	}
	if(-1==ReadJpegBuf(srcImage->srcdata, srcImage->iLen, bitmapBack))/*解码，灰度处理*/
	{
		return -1;
	}
#ifndef _DISABLE_OLD_ALG
	if(-1==VideoAlarm_threshold(bitmapBack)) /*二值化*/
	{
		return -1;
	}
#endif
	return 0;
}

/**
 * @brief 处理前景帧的解码、二值化操作
 * @author Summer.Wang
 * @return <失败返回-1><成功返回0>
 * @param[in] <srcImage><待处理的jpeg图像结构>
 * @param[in] <bitmapFore><处理后的图像位图结构>
 */
int VideoAlarm_getForeFrame(VA_SRCIMAGE *srcImage,VA_BITMAP *bitmapFore)
{
	if(srcImage->srcdata==NULL||bitmapFore->buf==NULL)
	{
		return -1;
	}
	if(-1==ReadJpegBuf(srcImage->srcdata, srcImage->iLen,bitmapFore))
	{
		return -1;
	}
#ifndef _DISABLE_OLD_ALG
	if(-1==VideoAlarm_threshold(bitmapFore))
	{
		return -1;
	}
#endif
	return bitmapFore->width;
}
/**
 * @brief 两灰度图像比较函数
 * @author Summer.Wang
 * @return 两图像不同的像素点数
 * @param[in] <bitmapBack><待处理的灰度图像位图结构>
 * @param[in] <bitmapFore><待处理的灰度图像位图结构>
 * @param[in] <self><查询模块属性>
 * @note Motion移植过来的新算法
 */
int VideoAlarm_comp_newer(VA_BITMAP *bitmapBack, VA_BITMAP *bitmapFore,struct VideoAlarm_t* self)
{
	int sum, i, count = 0;

	sum = bitmapBack->height * bitmapBack->width;

	for(i = 0; i < sum; i++)
	{
		register unsigned char diff = (int)(abs(*(bitmapBack->buf + i) - *(bitmapFore->buf + i)));
		if(diff > self->noise){
			count++;
		}
	}
// 强光过滤效果及处理有待考察
//	if(count > (sum * self->light / 100))
//	{
//		count = 0;
//	}
// 记得把之前不报警的bug处理移植过来
	return count;
}

/**
 * @brief 两位图图像比较函数
 * @author Summer.Wang
 * @return 两图像不同的像素点数
 * @param[in] <bitmapBack><待处理的图像位图结构>
 * @param[in] <bitmapFore><待处理的图像位图结构>
 * @param[in] <self><查询模块属性>
 * @note 这里有一个未知原因的bug修复，长期运行后会发生图像比较后该函数一直返回为0的情况，修正方法为重置分辨率
 */
int VideoAlarm_comp(VA_BITMAP *bitmapBack, VA_BITMAP *bitmapFore,struct VideoAlarm_t* self)
{
	int iCount=0;
	int iSum=0;
	int count=0;
#if 0
	int iRow=0;/*row*/
	int iColumn=0;/*column*/
	int iRowy=self->iAlarmRegionY;
	int iRowh=self->iAlarmRegionH;
	int iColumnx=self->iAlarmRegionX;
	int iColumnw=self->iAlarmRegionW;
	int ImageHeight=bitmapBack->height;
	int ImageWidth=bitmapBack->width;
	int pOffset=0;
	int RegionFlag=0;

	if( 0>self->iAlarmRegionY ||ImageHeight < self->iAlarmRegionY) /*判断是否图像越界*/
	{
		RegionFlag=1;
	}
	if( 0>self->iAlarmRegionX ||ImageWidth < self->iAlarmRegionX)
	{
		RegionFlag=1;
	}
	if(iRowy==0&&iRowh==0&&iColumnx==0&&iColumnw==0)
	{
		RegionFlag=1;
	}
#endif
//	if(RegionFlag)/*如果没有越界则按照正常的*/
//	{
	iSum=bitmapBack->height*bitmapBack->width;
	for(iCount=0;iCount<iSum;iCount++)
	{
		if(*(bitmapBack->buf+iCount)^*(bitmapFore->buf+iCount))
		{
			count++;
		}
	}
	//printf("\n\r                                                                                     the diff pixel count is %d\n",count);
	if(count == 0){//为了调试用，如果两个图像完全一样则停在这里不停打印
		self->count_noalarm_bug++;
		if(self->count_noalarm_bug >= 30){//发生不报警bug时，改变分辨率
			msg_container *msg = (msg_container*)self;
			VideoAlarm_Msg_Send(msg,MSG_VA_P_FRAME);
		}
	}
	else{
		self->count_noalarm_bug = 0;
	}
//	}
#if 0
	if(!RegionFlag)/*如果越界则把值强制转换成图像的最大的值*/
	{
		if(self->iAlarmRegionH>ImageHeight-iRowy)
		{
			iRowh=ImageHeight-iRowy;
		}
		if(self->iAlarmRegionW>ImageWidth-iColumnx)
		{
			iColumnw=ImageWidth-iColumnx;
		}
		for(iRow=iRowy;iRow<=iRowh;iRow++)
		{
			for(iColumn=iColumnx;iColumn<=iColumnw;iColumn++)
			{
				pOffset=iRow*ImageWidth+iColumn;
				if(*(bitmapBack->buf+pOffset)^*(bitmapFore->buf+pOffset))/*对比图像*/
				{
					count++;
				}
			}
		}
		RegionFlag=0;
	}
#endif
	return count;
}

/**
 * @brief 确定是否触发视频报警函数
 * @author Summer.Wang
 * @return 触发报警的标志位值
 * @param[in] <bitmapBack><待处理的图像位图结构>
 * @param[in] <bitmapFore><待处理的图像位图结构>
 * @param[in|out] <self><查询及控制模块属性>
 * @note 根据总像素点数、报警级别、不同的像素点数确定是否报警
 */
char VideoAlarm_ImageCmp(VA_BITMAP *bitmapBack, VA_BITMAP *bitmapFore,struct VideoAlarm_t* self)
{
	int iCount;
	int pixels = bitmapBack->height * bitmapBack->width;
	if(bitmapBack->buf==NULL||bitmapFore->buf==NULL)
	{
		return -1;
	}

#ifndef _DISABLE_OLD_ALG

	iCount=VideoAlarm_comp(bitmapBack, bitmapFore,self);/*比较两幅图*/

	switch(self->MotionSense){
		case 1:
			if(iCount>pixels*0.7)/*%70*/
			{
				self->alarm=1;
			}
			break;
		case 2:
			if(iCount>pixels*0.6)/*%60*/
			{
				self->alarm=1;
			}
			break;
		case 3:
			if(iCount>pixels*0.5)/*%50*/
			{
				self->alarm=1;
			}
			break;
		case 4:
			if(iCount>pixels*0.4)/*%40*/
			{
				self->alarm=1;
			}
			break;
		case 5:
			if(iCount>pixels*0.3)/*%30*/
			{
				self->alarm=1;
			}
			break;
		case 6:
			if(iCount>pixels*0.2)/*%20*/
			{
				self->alarm=1;
			}
			break;
		case 7:
			if(iCount>pixels*0.1)/*%10*/
			{
				self->alarm=1;
			}
			break;
		case 8:
			if(iCount>pixels*0.07)/*%7*/
			{
				self->alarm=1;
			}
			break;
		case 9:
			if(iCount>pixels*0.04)/*%4*/
			{
				self->alarm=1;
			}
			break;
		default:
			self->alarm=0;
			break;
	}

#else

	iCount = VideoAlarm_comp_newer(bitmapBack, bitmapFore,self);
	switch(self->MotionSense){
	case 1:
		if(iCount > pixels * 0.27)	self->alarm = 1;	//27% * 640*480 = 82944
		break;
	case 2:
		if(iCount > pixels * 0.20)	self->alarm = 1;	//20% * 640*480 = 61440
		break;
	case 3:
		if(iCount > pixels * 0.10)	self->alarm = 1;	//10% * 640*480 = 30720
		break;
	case 4:
		if(iCount > pixels * 0.06)	self->alarm = 1;	//6% * 640*480 = 18432
		break;
	case 5:
		if(iCount > pixels * 0.03)	self->alarm = 1;	//3% * 640*480 = 9216
		break;
	case 6:
		if(iCount > pixels * 0.022)	self->alarm = 1;	//2.2% * 640*480 = 6758.4
		break;
	case 7:
		if(iCount > pixels * 0.014)	self->alarm = 1;	//1.4% * 640*480 = 4300.8
		break;
	case 8:
		if(iCount > pixels * 0.009)	self->alarm = 1;	//0.9% * 640*480 = 2764.8
		break;
	case 9:
		if(iCount > pixels * 0.005)	self->alarm = 1;	//0.5% * 640*480 = 1536
		break;
	default:
		self->alarm = 0;
		break;
	}
#endif

	if(self->alarm == 1){
		VideoAlarm_Log_S("the number of different pixels when alarm is %d", iCount);
	}
	return self->alarm;
}

/**
 * @brief 背景处理函数
 * @author Summer.Wang
 * @return Null
 * @param[in] <srcImageBack><待处理的图像jpeg结构>
 * @param[in] <bitmapBack><被处理后的图像位图结构>
 * @param[in|out] <self><查询及控制模块属性>
 * @note 背景帧处理的逻辑控制
 */
void VideoAlarm_BackImageProcess(VA_SRCIMAGE *srcImageBack,VA_BITMAP *bitmapBack,struct VideoAlarm_t* self)
{
	if(self->getback){
		if(self->alarmswitch==1 && self->fetch==1){//报警开启且允许取帧
			if(self->Motormove==0 && self->noalarm == 0){//电机没转且没在报警后30秒
				if(self->holdback == 1){//持有一帧
					ReleaseNewestVideoFrame(srcImageBack->iIndex);
					self->holdback = 0;
				}
				int result = GetNewestVideoFrame(&srcImageBack->srcdata, &srcImageBack->iLen, &srcImageBack->iIndex);
				if(result == -1){
					return;
				}
				else if(srcImageBack->srcdata==NULL || srcImageBack->iLen<0){
//					self->holdback = 1;
					return;
				}
				else{//取帧成功
					self->holdback = 1;
					self->getfore = 0;
					int result_1 = VideoAlarm_getBackFrame(srcImageBack,bitmapBack);/*处理背景帧*/
					if(result_1 != -1){
						self->getback = 0;//只有处理成功后，才将此清零，否则不清零下次继续进入
					}
				}
			}
		}
	}
}

/**
 * @brief 前景处理和图像比较函数
 * @author Summer.Wang
 * @return Null
 * @param[in] <srcImageFore><待处理的图像jpeg结构>
 * @param[in] <bitmapBack><被处理后的图像位图结构>
 * @param[in] <bitmapFore><待比较的图像位图结构>
 * @param[in|out] <self><查询及控制模块属性>
 * @note 前景帧处理和图像比较的逻辑控制
 */
void VideoAlarm_ForeImageCompare(VA_SRCIMAGE *srcImageFore,VA_BITMAP *bitmapBack,VA_BITMAP *bitmapFore,struct VideoAlarm_t* self)
{
	if(self->getfore){
		if(self->alarmswitch==1 && self->fetch==1){//报警开启且允许取帧
			if(self->Motormove==0 && self->noalarm == 0){//电机没转且没在报警后30秒
				if(self->holdfore == 1){//持有一帧
					ReleaseNewestVideoFrame(srcImageFore->iIndex);
					self->holdfore = 0;
				}
				int result = GetNewestVideoFrame(&srcImageFore->srcdata, &srcImageFore->iLen, &srcImageFore->iIndex);
				if(result == -1){
					return;
				}
				else if(srcImageFore->srcdata==NULL || srcImageFore->iIndex<0 || srcImageFore->iLen==0){
//					self->holdfore = 1;
					return;
				}
				else{//取帧成功
					self->holdfore = 1;
					self->Pixels = VideoAlarm_getForeFrame(srcImageFore,bitmapFore);/*处理前景帧*/
					if(self->oldPixels != self->Pixels)/*检测是否有更换图像的分辨率，如果改变了就得重新取背景*/
					{
						self->oldPixels = self->Pixels;
						self->getback = 1;
					}
					else
					{
						self->alarm = VideoAlarm_ImageCmp(bitmapBack, bitmapFore, self);/*比较两幅图*/
						if(self->alarm_state==1 && self->alarm==0)//处于报警中，且本次循环没有触发报警
						{
							msg_container *msg = (msg_container*)self;
							self->alarm_state = 2;//报警状态为不处于报警中
							VideoAlarm_Msg_Send(msg,MSG_VA_P_ALARM);
						}
					}
					self->getfore = 0;
				}
			}
		}
	}
}

/**
 * @brief 将图片存储为文件的函数
 * @author Summer.Wang
 * @return Null
 * @param[in] <filename><为图像定义一个名字>
 * @param[in] <datapoint><指向图像的指针>
 * @param[in] <len><图像存储长度>
 * @param[in|out] <self><查询及控制模块属性>
 * @note 定义一个图像名，可以将图像存储为固定目录下的jpeg文件
 */
void VideoAlarm_StorePicToFile(char *filename,char *datapoint,int len,struct VideoAlarm_t* self)
{
	char tmp[60];
	memset(tmp,'\0',50);
	strcat(tmp,self->pathname);
//	strcat(tmp,"/");
	strcat(tmp,filename);
	strcat(tmp,".jpg");
	DEBUGPRINT(DEBUG_INFO, "VideoAlarm:the pathname is %s!\n",tmp);

	FILE *file = fopen(tmp, "w");
	if(file == NULL){
		return;
	}
	if(fwrite(datapoint, len, 1, file)!=1){
		fclose(file);
		file = NULL;
		return;
	}
	fclose(file);
	file = NULL;
}

/**
 * @brief 触发报警函数
 * @author Summer.Wang
 * @return Null
 * @param[in] <srcImageBack><待存储的图像结构>
 * @param[in] <srcImageFore><待存储的图像结构>
 * @param[in|out] <self><查询及控制模块属性>
 * @note 该模块被触发后，进行存储前景、背景帧的操作和交互控制
 */
void VideoAlarm_TriggerAlarm(VA_SRCIMAGE *srcImageBack,VA_SRCIMAGE *srcImageFore,struct VideoAlarm_t* self)
{
	char timestamp[20];
	time_t ts;

	if(self->alarm && self->fetch){
		DEBUGPRINT(DEBUG_INFO, "#####################ALARM####################\n");
		msg_container *msg = (msg_container*)self;
		if(self->alarm_type == 0)	VideoAlarm_Msg_Send(msg,MSG_VA_P_INFORM);//通知音频线程30内不报警

		self->count_store = 0;//每次进入报警模块要先把此清零,如果后续再改回存5张图片，则需修改此处，否则会造成图片不全
		self->store = 0;

		memset(self->pathname,'\0',60);//有可能在报警路径没上传后，就进入到另一次报警中，导致路径错误
		time(&ts);
		self->timestamp = (int)ts;

		if(access(VA_PIC_STORE,F_OK) == -1)//目录不存在
		{
			if(mkdir("/usr/share/alarmpic",S_IRWXU|S_IRWXG|S_IRWXO) == -1)	return;
		}
		sprintf(self->pathname, VA_PIC_STORE);
		sprintf(timestamp,"%ld",ts);
		strcat(self->pathname,timestamp);
		strcat(self->pathname,"/");
		if(mkdir(self->pathname,S_IRWXU|S_IRWXG|S_IRWXO) == -1){
			my_remove_file(self->pathname);
			self->alarm = 0;//在插入USB后的文件夹操作都返回失败，将已触发的报警信息清除
//			rmdir(self->pathname);
			return;
		}

		if(self->holdback == 0){
			int a = GetNewestVideoFrame(&srcImageBack->srcdata, &srcImageBack->iLen, &srcImageBack->iIndex);
			if(a == -1){
				return;
			}
			else if(srcImageBack->srcdata==NULL || srcImageBack->iIndex<0 || srcImageBack->iLen==0){
//				self->holdback = 1;
				return;
			}
			else self->holdback = 1;
			sleep(1);
		}
		VideoAlarm_StorePicToFile("1",srcImageBack->srcdata,srcImageBack->iLen,self);
		if(self->holdback == 1){
			ReleaseNewestVideoFrame(srcImageBack->iIndex);
			self->holdback = 0;
		}

		if(self->holdfore == 0){
			int b = GetNewestVideoFrame(&srcImageFore->srcdata, &srcImageFore->iLen, &srcImageFore->iIndex);
			if(b == -1){
				return;
			}
			else if(srcImageFore->srcdata==NULL || srcImageFore->iIndex<0 || srcImageFore->iLen==0){
//				self->holdfore = 1;
				return;
			}
			else self->holdfore = 1;
			sleep(1);
		}
		VideoAlarm_StorePicToFile("2",srcImageFore->srcdata,srcImageFore->iLen,self);
		if(self->holdfore == 1){
			ReleaseNewestVideoFrame(srcImageFore->iIndex);
			self->holdfore = 0;
		}

		if(self->alarm_type == 0){
			self->alarm_state = 1;//报警状态为处于报警中
			VideoAlarm_Msg_Send(msg,MSG_VA_P_ALARM);//只有在视频报警时，才给控制通道发消息
		}

		self->alarm = 0;
//		self->noalarm = 1;
		self->store = 1;
		//self->noalarm = 1;//需求更改：视频报警和音频报警后，都在30秒内不报警
		if(self->alarm_type == 0)	self->noalarm = 1;//报警类型为视频报警时，才触发30秒不报警模块
	}
}

/**
 * @brief 上传图片函数
 * @author Summer.Wang
 * @return Null
 * @param[in] <srcImage><待存储的图像结构>
 * @param[in|out] <self><查询及控制模块属性>
 * @note 每次循环可存储一张图片，并立即释放。
 */
void VideoAlarm_UploadPic(VA_SRCIMAGE *srcImage,struct VideoAlarm_t* self)
{
	char filename;

	if(self->store && self->fetch){
		sprintf(&filename, "%d",(self->count_store+3));

		int result = GetNewestVideoFrame(&srcImage->srcdata, &srcImage->iLen, &srcImage->iIndex);
		if(result == -1){
			return;
		}
		else if(srcImage->srcdata==NULL || srcImage->iIndex<0 || srcImage->iLen==0){
//			ReleaseNewestVideoFrame(srcImage->iIndex);
			return;
		}
		VideoAlarm_StorePicToFile(&filename,srcImage->srcdata,srcImage->iLen,self);
		ReleaseNewestVideoFrame(srcImage->iIndex);

#ifdef Http_LOG_ON
		videoalarm_write_log_info_to_file(self->pathname,"w");
		videoalarm_write_log_info_to_file("Three pictures have been written.\n","a+");
#endif

		self->count_store++;
		if(self->count_store >= 1){//存5张图片时，此处改为3；存3张图片时，此处为1。
			self->count_store = 0;
			self->store = 0;
			self->count_back = 0;//报警后，更换背景的同时，将定时更换背景清零
			self->getback = 1;

#ifdef Http_LOG_ON
			videoalarm_write_log_info_to_file("Send message to child process.\n","a+");
#endif
			msg_container *msg = (msg_container*)self;
			VideoAlarm_Msg_Send(msg,MSG_VA_P_FINISH);
		}
	}
}
/**
 * @brief 测试算法时写的测试函数
 * @author Summer.Wang
 * @return Null
 * @param[in|out] <void>
 */
void wangsu_test()
{
	VA_SRCIMAGE srcImage = {
		.iIndex = -1,
		.upIndex = -1,
		.iLen = 0,
		.srcdata = NULL,
	};
	VA_BITMAP bitmap;
	bitmap.buf = (unsigned char*)malloc(VA_BITMAP_SIZE);

	FILE *fileread = fopen("/mnt/image/2","r");
	if(fileread == NULL){
		return;
	}
	char data[30000];
	int count = fread(data, 1, 30000, fileread);
	fclose(fileread);

	srcImage.srcdata = &data;
	srcImage.iLen = count;

	VideoAlarm_getBackFrame(&srcImage,&bitmap);

	FILE *filewrite = fopen("/mnt/image/1.bmp", "w");
	if(filewrite == NULL){
		return;
	}

	int num = fwrite(bitmap.buf, 1, bitmap.height*bitmap.width, filewrite);
	fclose(filewrite);

	return NULL;
}

/**
 * @brief 打印信息写文件函数
 * @author Summer.Wang
 * @return Null
 * @param[out] <void>
 * @param[in] 要打印的字符串指针
 */
void videoalarm_write_log_info_to_file(char *pStrInfo,char *Oper)
{
	FILE *pFile = NULL;

	if ((pStrInfo == NULL) || (0 == strlen(pStrInfo)))
	{
		return;
	}

	pFile = fopen("/etc/alarminfo_20131124.txt",Oper);

	if (NULL == pFile)
	{
        return;
	}

	fprintf(pFile, pStrInfo);

	fclose(pFile);
}

/**
 * @brief 入口函数
 * @author Summer.Wang
 * @return Null
 * @param[in|out] <void>
 */
void *VideoAlarm_Thread(void *arg)
{
	VA_SRCIMAGE srcImageBack = {
		.iIndex = -1,
		.upIndex = -1,
		.iLen = 0,
		.srcdata = NULL,
	};
	VA_SRCIMAGE srcImageFore = {
		.iIndex = -1,
		.upIndex = -1,
		.iLen = 0,
		.srcdata = NULL,
	};
	VA_SRCIMAGE srcImage = {
		.iIndex = -1,
		.upIndex = -1,
		.iLen = 0,
		.srcdata = NULL,
	};
	VA_SRCIMAGE srcImageDetect = {
		.iIndex = -1,
		.upIndex = -1,
		.iLen = 0,
		.srcdata = NULL,
	};
	bitmapBack.buf = (unsigned char*)malloc(VA_BITMAP_SIZE);
	bitmapFore.buf = (unsigned char*)malloc(VA_BITMAP_SIZE);
	bitmapDetect.buf = (unsigned char*)malloc(VA_BITMAP_SIZE);

	VideoAlarm_list = (struct VideoAlarm_t *)malloc(sizeof(struct VideoAlarm_t));
	struct VideoAlarm_t *self = VideoAlarm_list;

	VideoAlarm_Module_Init((msg_container*)self);
	VideoAlarm_ThreadSync();
	VideoAlarm_Variable_Init(self);

	while(1)
	{
//		DEBUGPRINT(DEBUG_INFO, "the switch is %d,the fetch is %d,the alarmstate is %d",self->alarmswitch,self->fetch,self->alarm_state);
		VideoAlarm_ChangeBackImage(self);
		VideoAlarm_NoAlarmSection(self);
		VideoAlarm_Msg_Receive(self,&srcImageBack,&srcImageFore);
		child_setThreadStatus(VIDEO_ALARM_MSG_TYPE, NULL);
//		wangsu_test();
		VideoAlarm_DetectFrameFail(&srcImageDetect,&bitmapDetect,self);
		VideoAlarm_BackImageProcess(&srcImageBack,&bitmapBack,self);
		VideoAlarm_ForeImageCompare(&srcImageFore,&bitmapBack,&bitmapFore,self);
		VideoAlarm_TriggerAlarm(&srcImageBack,&srcImageFore,self);
		VideoAlarm_UploadPic(&srcImage,self);
		thread_usleep(300000);//每300ms运行线程
	}

	free(bitmapBack.buf);
	free(bitmapFore.buf);
	free(bitmapDetect.buf);
	free(self);
	return NULL;
}

