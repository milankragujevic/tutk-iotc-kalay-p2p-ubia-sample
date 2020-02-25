/**   @file []
   *  @brief AudioAlarm的实现
   *  @note
   *  @author:   wjn&wzq
   *  @date 2013-5-7
   *  @remarks modify record:增加注释
   *  @remarks 算法大修改，王素2014-5-7
*/
#include <stdio.h>
#include <stdlib.h>
#include <errno.h>
#include <time.h>
#include <math.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <netdb.h>
#include <string.h>
#include "fftw3.h"
#include "audioalarm_common.h"
#include <unistd.h>
//#include "common/logfile.h"
#include <complex.h>
#include "common/logfile.h"
#include "common/common.h"
#include "common/utils.h"
#include "common/thread.h"
#include "common/msg_queue.h"
#include "Audio/Audio.h"
#include "audioalarm.h"
#include "audioalarm_common.h"
#include "testcommon.h"
#include "VideoAlarm/VideoAlarm.h"
#include "common/logserver.h"
#include "queue.h"

#define CENTERNUM 24

struct AudioAlarmAlg{
	double sum;
	int	   num;
	double average;
};
struct SountAlgPara{
	float environment;//统计环境音时，差值小于该阈值的被包含在环境音中。参考值4(严格)-8(宽松)
	float noise_offset;//判断报警条件一，噪声大于环境音该阈值的符合条件。参考值4(宽松)-10(严格)
	float minus_amp;//判断报警条件二，差值大于该阈值的符合条件。参考值4(宽松)-10(严格)
	int count;//判断报警条件三，满足上面两个报警条件的数据个数(共10个)。参考值1(宽松)-10(严格)
};
spectrum *spectrumtmp = NULL;
fftw_complex *FFTCompxSRC, *out;
fftw_plan FFTPlan;
Queue *DbQueue;
char *pData = NULL;
float lastdb;
int count_poweron = 0;
struct AudioAlarmAlg alg_environment = {0.0,0,0.0};
struct SountAlgPara alg_parameter = {5.0,5.0,5.0,2};
//float fSinTab[FFT_N/4+1];       //定义正弦表的存放空间
//long minus_Stime, cost_Stime=0;	  //for yeting test
#ifdef _RECOARD_TEST_
long Scount=0;
#endif

int handleAudioAlarm(AUDIOALARM_FLAG *flag);
int AudioAlarmDetect( fftw_complex *compxSrc,char *pData,spectrum *spectrumtmp,int iLevel);
int  CountDb(fftw_complex *FFTCompxSRC,fftw_plan Plan,int iLevel,spectrum *spectrumtmp);
void fnSpect(fftw_complex *compxInput,spectrum *spectrumA);
float thirdOctave(spectrum *spectrumOut);
int alg_sound_dection(float db);
//void createSinTab(float *fSin);
//void FFT(compx *compxInput);

/*
 * 1 not found file 			---->Yes
 * 2 30` Audio Alarm later		---->NO
 * 3 App open AudioAlarmFlag	---->NO
 * 
 * return 1:alarm
 * return -1: no data
 * return -2: no alarm
 * */
int handleAudioAlarm(AUDIOALARM_FLAG *flag){
	int nRet = 0;
	int index = 0;
//	struct timeval Stime;
//	long time;

	nRet = GetAlarmAudioFrame(&pData,&index);

//	gettimeofday(&Stime,NULL);
//	time = ((long)Stime.tv_sec)*1000 + (long)Stime.tv_usec/1000;
//	if(cost_Stime == 0)	cost_Stime = time;
//	minus_Stime = time - cost_Stime;

	if(index == flag->last_index){
		return 0;
	}

#ifdef _RECOARD_TEST_
	FILE *file = fopen("/mnt/record.pcm","a+");
	fwrite(pData,2048,1,file);
	fclose(file);
	Scount += 1024;
#endif

	flag->last_index = index;
	if(nRet == -1){
		return -1;
	}else if(nRet == 0){
		if(0==AudioAlarmDetect(FFTCompxSRC,pData,spectrumtmp,flag->Level)){
			nRet = 1;
		}else{
			nRet = 2;
		}
	}

	return nRet;
}
/**
 *@brief 把数据转换成复数形式
 *@param  pData,数据源
 *@param   FFTCompxSRC 复数结构
 */
void Data2FFTComplex(fftw_complex *FFTCompxSRC,char * pData){
	int iCount = 0;
	short iTemp = 0;

	for(;iCount<FFT_N;iCount++){
		iTemp = 0;
		iTemp = pData[iCount*2];
		iTemp |= pData[iCount*2+1]<<8;
		FFTCompxSRC[iCount][0] = iTemp;
		FFTCompxSRC[iCount][1] = 0;
	}
}

/*
 * AudioAlarm 检测
 * */
int AudioAlarmDetect(fftw_complex *FFTCompxSRC,char *pData,spectrum *spectrumtmp,int iLevel)
{
	//PCM数据源复数转化
	Data2FFTComplex(FFTCompxSRC,pData);
	if(CountDb(FFTCompxSRC,FFTPlan,iLevel,spectrumtmp))
	{
		return 0;
	}else
	{
		return -1;
	}

}

/*  计算分贝
 *
 * */
int  CountDb(fftw_complex *FFTCompxSRC,fftw_plan Plan,int iLevel,spectrum *spectrumtmp)
{
	float fLp;
	int iTmp = NOALARM;

	//使报警发生的更容易，则环境音要把握得更严格（小），噪声差值要更小
	switch(iLevel){
	case 1:
		alg_parameter.environment = 5.2;
		alg_parameter.noise_offset = 15.0;
		alg_parameter.count = 2;
		break;
	case 2:
		alg_parameter.environment = 4.8;
		alg_parameter.noise_offset = 13.0;
		alg_parameter.count = 2;
		break;
	case 3:
		alg_parameter.environment = 4.4;
		alg_parameter.noise_offset = 10.0;//9.0
		alg_parameter.count = 2;
		break;
	case 4:
		alg_parameter.environment = 4.0;
		alg_parameter.noise_offset = 8.0;
		alg_parameter.count = 2;
		break;
	case 5:
		alg_parameter.environment = 3.6;
		alg_parameter.noise_offset = 7.0;
		alg_parameter.count = 2;
		break;
	case 6:
		alg_parameter.environment = 3.2;
		alg_parameter.noise_offset = 6.0;
		alg_parameter.count = 2;
		break;
	case 7:
		alg_parameter.environment = 2.8;
		alg_parameter.noise_offset = 5.3;
		alg_parameter.count = 2;
		break;
	case 8:
		alg_parameter.environment = 2.4;
		alg_parameter.noise_offset = 4.5;
		alg_parameter.count = 2;
		break;
	case 9:
		alg_parameter.environment = 2.0;
		alg_parameter.noise_offset = 3.0;
		alg_parameter.count = 1;
		break;
	default:
		break;
	}

	fftw_execute(Plan); // 执行变换
  	fnSpect(out,spectrumtmp);//生成频谱
  	fLp=thirdOctave(spectrumtmp);//声压级及A加权
  	iTmp = alg_sound_dection(fLp);//报警算法
  	return iTmp;
}

/*******************************************************************
函数原型：struct compxEE compxEE(struct compxEE compxEEA,struct compxEE compxEEB)
函数功能：对两个复数进行乘法运算
输入参数：两个以联合体定义的复数compxEEA,compxEEB
输出参数：compxEEA和compxEEB的乘积，以联合体的形式输出
*******************************************************************/
//struct compxEE compxEE(struct compxEE compxEEA,struct compxEE compxEEB)
//{
//	struct compxEE compxEEC;
//	compxEEC.fReal=compxEEA.fReal*compxEEB.fReal-compxEEA.fImag*compxEEB.fImag;
// 	compxEEC.fImag=compxEEA.fReal*compxEEB.fImag+compxEEA.fImag*compxEEB.fReal;
// 	return(compxEEC);
//}

/******************************************************************
函数原型：void createSinTab(float *fSin)
函数功能：创建一个正弦采样表，采样点数与福利叶变换点数相同
输入参数：*fSin存放正弦表的数组指针
输出参数：无
******************************************************************/
//void createSinTab(float *fSin)
//{
//	int iCount;
//  	for(iCount=0;iCount<=FFT_N/4;iCount++)
//  	{
//  		fSin[iCount]=sin(2*PI*iCount/FFT_N);
//  	}
//}

/******************************************************************
函数原型：void sinTab(float fPi)
函数功能：采用查表的方法计算一个数的正弦值
输入参数：fPi 所要计算正弦值弧度值，范围0--2*PI，不满足时需要转换
输出参数：输入值fPi的正弦值
******************************************************************/
//float sinTab(float fPi)
//{
//	int iNum;
//  	float fTmp = 0;
//   	iNum=(int)(fPi*FFT_N/2/PI);
//
//  	if(iNum>=0&&iNum<=FFT_N/4)
//  	{
//    	fTmp=fSinTab[iNum];
//  	}
//  	else if(iNum>FFT_N/4&&iNum<FFT_N/2)
//    {
//     	iNum-=FFT_N/4;
//     	fTmp=fSinTab[FFT_N/4-iNum];
//    }
//  	else if(iNum>=FFT_N/2&&iNum<3*FFT_N/4)
//    {
//     	iNum-=FFT_N/2;
//     	fTmp=-fSinTab[iNum];
//  	}
//  	else if(iNum>=3*FFT_N/4&&iNum<3*FFT_N)
//    {
//     	iNum=FFT_N-iNum;
//     	fTmp=-fSinTab[iNum];
//   	}
//
//  	return fTmp;
//}
/******************************************************************
函数原型：void cosTab(float fPi)
函数功能：采用查表的方法计算一个数的余弦值
输入参数：fPi 所要计算余弦值弧度值，范围0--2*PI，不满足时需要转换
输出参数：输入值fPi的余弦值
******************************************************************/
//float cosTab(float fPi)
//{
//	float fValue,fTmp;
//   	fTmp=fPi+PI/2;
//   	if(fTmp>2*PI)
//   	{
//     	fTmp-=2*PI;
//   	}
//   	fValue=sinTab(fTmp);
//   	return fValue;
//}
/*****************************************************************
函数原型：void FFT(struct compx *compxInput)
函数功能：对输入的复数组进行快速傅里叶变换（FFT）
输入参数：*compxInput复数结构体组的首地址指针，struct compx型；
输出参数：无
*****************************************************************/
//void FFT(compx *compxInput)
//{
//	int iStage,iHalf,iFn,iCounti,iCountk,iCountl,iCountj=0;
//	int iLength,iLei,ip;
//	int iFFT=FFT_N;
//  	struct compxEE compxEEU,compxEEW,compxEET;
//
//   	iHalf=FFT_N/2;                  //变址运算，即把自然顺序变成倒位序，采用雷德算法
//   	iFn=FFT_N-1;
//   	for(iCounti=0;iCounti<iFn;iCounti++)
//   	{
//    		if(iCounti<iCountj)                    //如果iCounti<iCountj,即进行变址
//     		{
//      			compxEET.fReal=compxInput->fReal[iCountj];
//      			compxInput->fReal[iCountj]=compxInput->fReal[iCounti];
//      			compxInput->fReal[iCounti]=compxEET.fReal;
//     		}
//    		iCountk=iHalf;                    //求j的下一个倒位序
//    		while(iCountk<=iCountj)               //如果iCountk<=iCountj,表示iCountj的最高位为1
//     		{
//      			iCountj=iCountj-iCountk;                 //把最高位变成0
//      			iCountk=iCountk/2;                 //iCountk/2，比较次高位，依次类推，逐个比较，直到某个位为0
//    	 	}
//   		iCountj=iCountj+iCountk;                   //把0改为1
//  	}
// //FFT运算核，使用蝶形运算完成FFT运算
//	for(iCountl=1;(iFFT=iFFT/2)!=1;iCountl++)  //计算iCountl的值，即计算蝶形级数
//	{
//		;
//	}
//
//  	for(iStage=1;iStage<=iCountl;iStage++)                         // 控制蝶形结级数
//   	{                                        //iStage表示第iStage级蝶形，iCountl为蝶形级总数iCountl=log（2）N
//    		iLength=2<<(iStage-1);                            //iLength蝶形结距离，即第m级蝶形的蝶形结相距le点
//    		iLei=iLength/2;                               //同一蝶形结中参加运算的两点的距离
//    		compxEEU.fReal=1.0;                             //compxU为蝶形结运算系数，初始值为1
//    		compxEEU.fImag=0.0;
//    		compxEEW.fReal=cosTab(PI/iLei);                //compxW为系数商，即当前系数与前一个系数的商
//    		compxEEW.fImag=-sinTab(PI/iLei);
//    		for(iCountj=0;iCountj<=iLei-1;iCountj++)                  //控制计算不同种蝶形结，即计算系数不同的蝶形结
//    		{
//     			for(iCounti=iCountj;iCounti<=FFT_N-1;iCounti=iCounti+iLength)           //控制同一蝶形结运算，即计算系数相同蝶形结
//      			{
//        			ip=iCounti+iLei;                          //iCounti，ip分别表示参加蝶形运算的两个节点
//					compxEET.fReal=compxInput->fReal[ip];
//					compxEET.fImag=compxInput->fImag[ip];
//					compxEET=compxEE(compxEET,compxEEU);                   //蝶形运算，详见公式
//        			compxInput->fReal[ip]=compxInput->fReal[iCounti]-compxEET.fReal;
//        			compxInput->fImag[ip]=compxInput->fImag[iCounti]-compxEET.fImag;
//        			compxInput->fReal[iCounti]=compxInput->fReal[iCounti]+compxEET.fReal;
//        			compxInput->fImag[iCounti]=compxInput->fImag[iCounti]+compxEET.fImag;
//       			}
//      			compxEEU=compxEE(compxEEU,compxEEW);                          //改变系数，进行下一个蝶形运算
//     		}
//   	}
// }




/**
 *@brief 计算频率分布，和各频率的振幅。即频谱统计。
 *@param compxInput FFT之后的一个复数结构体数组地址
 *@param spectrumA 频谱统计结构体数组地址。
 */
void fnSpect(fftw_complex *compxInput,spectrum *spectrumA)
{
	int iCount;
	float fTmpreal;
//	spectrum *spectrumA=(spectrum*)malloc((FFT_N/2)*sizeof(spectrum));

	for(iCount=0;iCount<FFT_N/2;iCount++)					//求各频率校准后的振幅值以相应的频率；
	{
		fTmpreal=sqrt(compxInput[iCount][0]*compxInput[iCount][0]+compxInput[iCount][1]*compxInput[iCount][1]);   //求幅值，复数的模；
		if(iCount==0)
		{
			spectrumA[iCount].fAmplitude=fTmpreal/FFT_N;		//直流分量幅值；
			spectrumA[iCount].fAmplitude=fabs(spectrumA[iCount].fAmplitude);
			spectrumA[iCount].fFrequency=0;

		}else
		{
			spectrumA[iCount].fAmplitude=2*fTmpreal/FFT_N;		//校准幅值公式；
			spectrumA[iCount].fAmplitude=fabs(spectrumA[iCount].fAmplitude);
			spectrumA[iCount].fFrequency=(iCount*SF)/FFT_N;			//求对应的频率值；
		}
	}
}


/**
 *@brief 求各频带内的能量值，再计算频带内的声压级，分贝值，最后进行1/3倍频程校准。
 *@param spectrumOut 频谱统计结构体数组地址。
 *@return 分贝值。
 */
float thirdOctave(spectrum *spectrumOut)
{
	int iCount;               //计数器
	float ftmp=0.0;        //总声压级
	int freNumCount[CENTERNUM]={0};
	spectrum spectrumTal[CENTERNUM]={
		{0.0,20.0},{0.0,25.0},{0.0,31.5},{0.0,40.0},{0.0,50.0},
		{0.0,63.0},{0.0,80.0},{0.0,100.0},{0.0,125.0},{0.0,160.0},
		{0.0,200.0},{0.0,250.0},{0.0,315.0},{0.0,400.0},{0.0,500.0},
		{0.0,630.0},{0.0,800.0},{0.0,1000.0},{0.0,1250.0},{0.0,1600.0},
		{0.0,2000.0},{0.0,2500.0},{0.0,3150.0},{0.0,4000.0},};
	//	{0.0,5000.0},{0.0,6300.0},{0.0,8000.0},};
	//	{0.0,10000.0},{0.0,12500.0},{0.0,16000.0},{0.0,20000.0},
	//};
	for(iCount=0;iCount<FFT_N/2;iCount++)   //进行1/3倍频程滤波，并计算各频带内的能量值；
	{
		if(spectrumOut[iCount].fFrequency>17.8 && spectrumOut[iCount].fFrequency<=22.4)
		{
			spectrumTal[0].fAmplitude+=spectrumOut[iCount].fAmplitude * spectrumOut[iCount].fAmplitude;//*spectrumOut[iCount].fAmplitude;
			freNumCount[0]++;
		}
		else if(spectrumOut[iCount].fFrequency>22.4 && spectrumOut[iCount].fFrequency<=28.2)
		{
			spectrumTal[1].fAmplitude+=spectrumOut[iCount].fAmplitude * spectrumOut[iCount].fAmplitude;//*spectrumOut[iCount].fAmplitude;
			freNumCount[1]++;
		}
		else if(spectrumOut[iCount].fFrequency>28.2 && spectrumOut[iCount].fFrequency<=35.5)
		{
			spectrumTal[2].fAmplitude+=spectrumOut[iCount].fAmplitude * spectrumOut[iCount].fAmplitude;//*spectrumOut[iCount].fAmplitude;
			freNumCount[2]++;
		}
		else if(spectrumOut[iCount].fFrequency>35.5 && spectrumOut[iCount].fFrequency<=44.7)
		{
			spectrumTal[3].fAmplitude+=spectrumOut[iCount].fAmplitude * spectrumOut[iCount].fAmplitude;//*spectrumOut[iCount].fAmplitude;
			freNumCount[3]++;
		}
		else if(spectrumOut[iCount].fFrequency>44.7 && spectrumOut[iCount].fFrequency<=56.2)
		{
			spectrumTal[4].fAmplitude+=spectrumOut[iCount].fAmplitude * spectrumOut[iCount].fAmplitude;//*spectrumOut[iCount].fAmplitude;
			freNumCount[4]++;
		}
		else if(spectrumOut[iCount].fFrequency>56.2 && spectrumOut[iCount].fFrequency<=70.8)
		{
			spectrumTal[5].fAmplitude+=spectrumOut[iCount].fAmplitude * spectrumOut[iCount].fAmplitude;//*spectrumOut[iCount].fAmplitude;
			freNumCount[5]++;
		}
		else if(spectrumOut[iCount].fFrequency>70.8 && spectrumOut[iCount].fFrequency<=89.1)
		{
			spectrumTal[6].fAmplitude+=spectrumOut[iCount].fAmplitude * spectrumOut[iCount].fAmplitude;//*spectrumOut[iCount].fAmplitude;
			freNumCount[6]++;
		}
		else if(spectrumOut[iCount].fFrequency>89.1 && spectrumOut[iCount].fFrequency<=112.2)
		{
			spectrumTal[7].fAmplitude+=spectrumOut[iCount].fAmplitude * spectrumOut[iCount].fAmplitude;//*spectrumOut[iCount].fAmplitude;
			freNumCount[7]++;
		}
		else if(spectrumOut[iCount].fFrequency>112.2 && spectrumOut[iCount].fFrequency<=141.2)
		{
			spectrumTal[8].fAmplitude+=spectrumOut[iCount].fAmplitude * spectrumOut[iCount].fAmplitude;//*spectrumOut[iCount].fAmplitude;
			freNumCount[8]++;
		}
		else if(spectrumOut[iCount].fFrequency>141.2 && spectrumOut[iCount].fFrequency<=177.8)
		{
			spectrumTal[9].fAmplitude+=spectrumOut[iCount].fAmplitude * spectrumOut[iCount].fAmplitude;//*spectrumOut[iCount].fAmplitude;
			freNumCount[9]++;
		}
		else if(spectrumOut[iCount].fFrequency>177.8 && spectrumOut[iCount].fFrequency<=223.9)
		{
			spectrumTal[10].fAmplitude+=spectrumOut[iCount].fAmplitude * spectrumOut[iCount].fAmplitude;//*spectrumOut[iCount].fAmplitude;
			freNumCount[10]++;
		}
		else if(spectrumOut[iCount].fFrequency>223.9 && spectrumOut[iCount].fFrequency<=281.8)
		{
			spectrumTal[11].fAmplitude+=spectrumOut[iCount].fAmplitude * spectrumOut[iCount].fAmplitude;//*spectrumOut[iCount].fAmplitude;
			freNumCount[11]++;
		}
		else if(spectrumOut[iCount].fFrequency>281.8 && spectrumOut[iCount].fFrequency<=354.8)
		{
			spectrumTal[12].fAmplitude+=spectrumOut[iCount].fAmplitude * spectrumOut[iCount].fAmplitude;//*spectrumOut[iCount].fAmplitude;
			freNumCount[12]++;
		}
		else if(spectrumOut[iCount].fFrequency>354.8 && spectrumOut[iCount].fFrequency<=446.7)
		{
			spectrumTal[13].fAmplitude+=spectrumOut[iCount].fAmplitude * spectrumOut[iCount].fAmplitude;//*spectrumOut[iCount].fAmplitude;
			freNumCount[13]++;
		}
		else if(spectrumOut[iCount].fFrequency>446.7 && spectrumOut[iCount].fFrequency<=562.3)
		{
			spectrumTal[14].fAmplitude+=spectrumOut[iCount].fAmplitude * spectrumOut[iCount].fAmplitude;//*spectrumOut[iCount].fAmplitude;
			freNumCount[14]++;
		}
		else if(spectrumOut[iCount].fFrequency>562.3 && spectrumOut[iCount].fFrequency<=707.9)
		{
			spectrumTal[15].fAmplitude+=spectrumOut[iCount].fAmplitude * spectrumOut[iCount].fAmplitude;//*spectrumOut[iCount].fAmplitude;
			freNumCount[15]++;
		}
		else if(spectrumOut[iCount].fFrequency>707.9 && spectrumOut[iCount].fFrequency<=891.3)
		{
			spectrumTal[16].fAmplitude+=spectrumOut[iCount].fAmplitude;//*spectrumOut[iCount].fAmplitude;
			freNumCount[16]++;
		}
		else if(spectrumOut[iCount].fFrequency>891.3 && spectrumOut[iCount].fFrequency<=1121.0)
		{
			spectrumTal[17].fAmplitude+=spectrumOut[iCount].fAmplitude * spectrumOut[iCount].fAmplitude;//*spectrumOut[iCount].fAmplitude;
			freNumCount[17]++;
		}
		else if(spectrumOut[iCount].fFrequency>1121.0 && spectrumOut[iCount].fFrequency<=1412.5)
		{
			spectrumTal[18].fAmplitude+=spectrumOut[iCount].fAmplitude * spectrumOut[iCount].fAmplitude;//*spectrumOut[iCount].fAmplitude;
			freNumCount[18]++;
		}
		else if(spectrumOut[iCount].fFrequency>1412.5 && spectrumOut[iCount].fFrequency<=1778.3)
		{
			spectrumTal[19].fAmplitude+=spectrumOut[iCount].fAmplitude * spectrumOut[iCount].fAmplitude;//*spectrumOut[iCount].fAmplitude;
			freNumCount[19]++;
		}
		else if(spectrumOut[iCount].fFrequency>1778.3 && spectrumOut[iCount].fFrequency<=2238.8)
		{
			spectrumTal[20].fAmplitude+=spectrumOut[iCount].fAmplitude * spectrumOut[iCount].fAmplitude;//*spectrumOut[iCount].fAmplitude;
			freNumCount[20]++;
		}
		else if(spectrumOut[iCount].fFrequency>2238.8 && spectrumOut[iCount].fFrequency<=2818.4)
		{
			spectrumTal[21].fAmplitude+=spectrumOut[iCount].fAmplitude * spectrumOut[iCount].fAmplitude;//*spectrumOut[iCount].fAmplitude;
			freNumCount[21]++;
		}
		else if(spectrumOut[iCount].fFrequency>2818.4 && spectrumOut[iCount].fFrequency<=3548.1)
		{
			spectrumTal[22].fAmplitude+=spectrumOut[iCount].fAmplitude * spectrumOut[iCount].fAmplitude;//*spectrumOut[iCount].fAmplitude;
			freNumCount[22]++;
		}
		else if(spectrumOut[iCount].fFrequency>3548.1 && spectrumOut[iCount].fFrequency<=4466.9)
		{
			spectrumTal[23].fAmplitude+=spectrumOut[iCount].fAmplitude * spectrumOut[iCount].fAmplitude;//*spectrumOut[iCount].fAmplitude;
			freNumCount[23]++;
		}
/*		else if(spectrumOut[iCount].fFrequency>4466.9 && spectrumOut[iCount].fFrequency<=5623.4)
		{
			spectrumTal[24].fAmplitude+=spectrumOut[iCount].fAmplitude*spectrumOut[iCount].fAmplitude;
		}
		else if(spectrumOut[iCount].fFrequency>5623.4 && spectrumOut[iCount].fFrequency<=7079.5)
		{
			spectrumTal[25].fAmplitude+=spectrumOut[iCount].fAmplitude*spectrumOut[iCount].fAmplitude;
		}
		else if(spectrumOut[iCount].fFrequency>7079.5 && spectrumOut[iCount].fFrequency<=8912.5)
		{
			spectrumTal[26].fAmplitude+=spectrumOut[iCount].fAmplitude*spectrumOut[iCount].fAmplitude;
		}*/
	}

	for(iCount=0;iCount<CENTERNUM;iCount++)//计算个频带内的声压级，及分贝值，并A记权声级；
	{
//		spectrumTal[iCount].fAmplitude=10*log10(spectrumTal[iCount].fAmplitude/(PO*PO));
		if(freNumCount[iCount] == 0){
			continue;
		}
		spectrumTal[iCount].fAmplitude = 10 * log10(spectrumTal[iCount].fAmplitude / freNumCount[iCount]);

		switch((int)(spectrumTal[iCount].fFrequency)){
		case 20:
			spectrumTal[iCount].fAmplitude-=50.5;
			break;
		case 25:
			spectrumTal[iCount].fAmplitude-=44.7;
			break;
		case 31:
			spectrumTal[iCount].fAmplitude-=39.4;
			break;
		case 40:
			spectrumTal[iCount].fAmplitude-=34.6;
			break;
		case 50:
			spectrumTal[iCount].fAmplitude-=30.2;
			break;
		case 63:
			spectrumTal[iCount].fAmplitude-=26.2;
			break;
		case 80:
			spectrumTal[iCount].fAmplitude-=22.5;
			break;
		case 100:
			spectrumTal[iCount].fAmplitude-=19.1;
			break;
		case 125:
			spectrumTal[iCount].fAmplitude-=16.1;
			break;
		case 160:
			spectrumTal[iCount].fAmplitude-=13.4;
			break;
		case 200:
			spectrumTal[iCount].fAmplitude-=10.4;
			break;
		case 250:
			spectrumTal[iCount].fAmplitude-=8.6;
			break;
		case 315:
			spectrumTal[iCount].fAmplitude-=6.6;
			break;
		case 400:
			spectrumTal[iCount].fAmplitude-=4.8;
			break;
		case 500:
			spectrumTal[iCount].fAmplitude-=3.2;
			break;
		case 630:
			spectrumTal[iCount].fAmplitude-=1.9;
			break;
		case 800:
			spectrumTal[iCount].fAmplitude-=0.8;
			break;
		case 1000:
			spectrumTal[iCount].fAmplitude-=0;
			break;
		case 1250:
			spectrumTal[iCount].fAmplitude+=0.6;
			break;
		case 1600:
			spectrumTal[iCount].fAmplitude+=1.0;
			break;
		case 2000:
			spectrumTal[iCount].fAmplitude+=1.2;
			break;
		case 2500:
			spectrumTal[iCount].fAmplitude+=1.3;
			break;
		case 3150:
			spectrumTal[iCount].fAmplitude+=1.2;
			break;
		case 4000:
			spectrumTal[iCount].fAmplitude+=1.0;
			break;
		case 5000:
			spectrumTal[iCount].fAmplitude+=0.5;
			break;
		case 6300:
			spectrumTal[iCount].fAmplitude-=0.1;
			break;
		case 8000:
			spectrumTal[iCount].fAmplitude-=1.1;
			break;
		default:
			break;
		}

		spectrumTal[iCount].fAmplitude=pow(10.0,spectrumTal[iCount].fAmplitude/10);

		ftmp+=spectrumTal[iCount].fAmplitude;
	}

	ftmp=10*log10(ftmp);
	printf("                                                                                    ftmp is:%.2f\n",ftmp);
#ifdef _RECOARD_TEST_
	char buf[100];
	FILE *file = fopen("/mnt/data.txt","a+");
	sprintf(buf,"%d-%d - %f\n",Scount-1024,Scount-1024+255,ftmp);
	fwrite(buf,strlen(buf),1,file);
	fclose(file);
	printf("                                                                                    < %d  %d>  -- ftmp is:%f\n",Scount-1024,Scount-1024+255,ftmp);
#endif
	return ftmp;    //返回总声压级；
}

/*1 for alarm, 0 presents noalarm*/
int alg_sound_dection(float db)
{
	float minus;
	PNode pnode;
	int i,count=0;
	int around;
	time_t ts;

	/*开机启动音影响DB值，抛弃若干*/
	count_poweron++;
	if(count_poweron < 5){
		lastdb = db;
		return 0;
	}

	/*存储大量合理DB数据，逐渐被优化*/
	minus = db - lastdb;//如果分贝相比之前大幅提高，需滤掉；以此来提出环境音。
	lastdb = db;
	if(minus < alg_parameter.environment){
		alg_environment.num++;
		alg_environment.sum+=db;
		alg_environment.average = alg_environment.sum/alg_environment.num;
	}
	printf("%d -> %f -> %f\n",alg_environment.num,alg_environment.sum,alg_environment.average);

	/*实验结果，控制误报，可根据实验优化*/
	if(alg_environment.average < 50.0)			around = 54;//安静环境下，54以上不误报
	else if(alg_environment.average >= 50.0) 	around = 55;//喧闹环境下，55以上不误报

	/*控制使队列中有15个分贝数据*/
	EnQueue(DbQueue,db);//幅值//和波动
	while(DbQueue->size > 15)
	{
		DeQueue(DbQueue,NULL);
	}

	/*查询最近的十个数据中是否符合报警条件*/
    i = DbQueue->size;
    pnode = DbQueue->front;
	while(i--)
	{
		if(pnode->data > alg_environment.average + alg_parameter.noise_offset &&
		   pnode->data > around)
//			&& pnode->data_2 > alg_parameter.minus_amp)
		{
			time(&ts);
			printf("                                        %d...%f\n",(int)ts,pnode->data);
			count++;
		}
		pnode = pnode->next;
	}
	printf("\n");
	/*根据满足报警条件的数据组个数触发报警*/
	if(count >= alg_parameter.count){
		ClearQueue(DbQueue);
		printf("                                                              ##### ALARM #####\n");
		return 1;
	}
	return 0;
}


int audio_variable_init(void)
{
	spectrumtmp=(spectrum*)malloc((FFT_N/2)*sizeof(spectrum));
	FFTCompxSRC = (fftw_complex*) fftw_malloc(sizeof(fftw_complex) * FFT_N);
	out = (fftw_complex*) fftw_malloc(sizeof(fftw_complex) * FFT_N);
	FFTPlan = fftw_plan_dft_1d(FFT_N, FFTCompxSRC, out, FFTW_FORWARD,FFTW_ESTIMATE);
	DbQueue = InitQueue();
	return 0;
}
