/**   @file []
   *  @brief    公用flash配置获取
   *  @note
   *  @author   wangfengbo
   *  @date     2013-9-2
   *  @remarks  增加注释
*/
/*******************************************************************************/
/*                                    头文件                                         */
/*******************************************************************************/
#include <sys/types.h>
#include <sys/stat.h>
#include <stdlib.h>
#include <unistd.h>
#include <stdio.h>
#include <string.h>
#include <ctype.h>
#include <malloc.h>
#include <errno.h>
#include <semaphore.h>
#include <logfile.h>
#include <stdio.h>

#include "utils.h"
#include "common_config.h"
#include "logfile.h"
#include "common.h"


/*******************************************************************************/
/*                                  宏定义区                                         */
/*******************************************************************************/
/** 配置项名称长度 */
#define ITEM_NAME_MAX_LEN           63

/** 配置项内容长度 */
#define ITEM_VALUE_MAX_LEN          255

#define ITEM_INFO_MAX_LEN           (ITEM_NAME_MAX_LEN+ITEM_VALUE_MAX_LEN+64)

/** 设置config到flash指令 */
#define WIRTEFLASHCMD               "nvram_set 2860 \"%s\"  \"%s\""

/** 清空config Item */
#define CLEAR_FLASH_CMD             "nvram_set 2860 \"%s\"  \"\""

/** pb_key 长度 */
#define PB_KEY_LEN                  176

/** 配置信息的版本号 注意这个要和sourec中的一致 若不一致则要更新 */
#define CONFIG_INFO_VERSION         "1.0.0.4"


/*******************************************************************************/
/*                             结构体及全局变量区                                     */
/*******************************************************************************/
/** 定义配置信息结构体 */
typedef struct _tag_config_item_info_
{
	char acItemName[ITEM_NAME_MAX_LEN+1];   ///< 配置项名字
	char acOldValue[ITEM_VALUE_MAX_LEN+1];  ///< 旧的值
	char acNewValue[ITEM_VALUE_MAX_LEN+1];  ///< 新的值
}config_item_info_t;

/** 定义  这个结构体的名字很纠结啊 */
typedef struct _tag_common_info_
{
	char acMacAddrBuf[ITEM_VALUE_MAX_LEN];
	char acPbKey[2*(ITEM_VALUE_MAX_LEN+1)];
}common_info_t;

/** 信号量，用来对共享资源的互斥 */
sem_t semConfigfile;

/** 配置信息表  下面的顺序不可以改变  */
config_item_info_t g_stConfigTable[TABLE_ITEM_NUM]=
{
	{"Wifi_Mode",                   {'\0'}, {'\0'}},
	{"Wifi_Active",                 {'\0'}, {'\0'}},
	{"Wifi_IP",                     {'\0'}, {'\0'}},
	{"Wifi_Subnet",                 {'\0'}, {'\0'}},
	{"Wifi_Gateway",                {'\0'}, {'\0'}},
	{"Wifi_DNS",                    {'\0'}, {'\0'}},
	{"Nettransmit_port",            {'\0'}, {'\0'}},
	{"HTTP_port",                   {'\0'}, {'\0'}},
	{"AutoChannelSelect",           {'\0'}, {'\0'}},
	{"Channel",                     {'\0'}, {'\0'}},
	{"UPnP_Use_IGD",                {'\0'}, {'\0'}},
	{"Wired_Mode",                  {'\0'}, {'\0'}},
	{"Wired_IP",                    {'\0'}, {'\0'}},
	{"Wired_Subnet",                {'\0'}, {'\0'}},
	{"Wired_Gateway",               {'\0'}, {'\0'}},
	{"Wired_DNS",                   {'\0'}, {'\0'}},
	{"Alarm_Motion",                {'\0'}, {'\0'}},
	{"Alarm_Motion_Sensitivity",    {'\0'}, {'\0'}},
	{"Alarm_Motion_Region",         {'\0'}, {'\0'}},
	{"Alarm_Audio",                 {'\0'}, {'\0'}},
	{"Alarm_Audio_Sensitivity",     {'\0'}, {'\0'}},
	{"Alarm_Battery",               {'\0'}, {'\0'}},
	{"Alarm_Image_Upload",          {'\0'}, {'\0'}},
	{"Alarm_Video_Upload",          {'\0'}, {'\0'}},
	{"Alarm_Start_time",            {'\0'}, {'\0'}},
	{"Alarm_End_Time",              {'\0'}, {'\0'}},
	{"NTPSync",                     {'\0'}, {'\0'}},
	{"NTPServerIP",                 {'\0'}, {'\0'}},
	{"Device_Serial",               {'\0'}, {'\0'}},
	{"System_Firmware_Version",     {'\0'}, {'\0'}},
	{"App_Firmware_Version",        {'\0'}, {'\0'}},
	{"Light_Net",                   {'\0'}, {'\0'}},
	{"Light_Power",                 {'\0'}, {'\0'}},
	{"Light_Night",                 {'\0'}, {'\0'}},
	{"WiFi_EnrType",                {'\0'}, {'\0'}},
	{"WiFi_SSID",                   {'\0'}, {'\0'}},
	{"WiFi_Passwd",                 {'\0'}, {'\0'}},
	{"WiFi_KeyID",                  {'\0'}, {'\0'}},
	{"Conf_Version",                {'\0'}, {'\0'}},
	{"Camera_Type",                 {'\0'}, {'\0'}},
	{"Camera_EncrytionID",          {'\0'}, {'\0'}},
	{"Server_URL",                  {'\0'}, {'\0'}},
	{"Video_Size",                  {'\0'}, {'\0'}},
	{"Video_Bright",                {'\0'}, {'\0'}},
	{"Video_Constract",             {'\0'}, {'\0'}},
	{"Video_Hflip",                 {'\0'}, {'\0'}},
	{"Video_Vflip",                 {'\0'}, {'\0'}},
	{"Video_Rate",                  {'\0'}, {'\0'}},
	{"Video_Simple",                {'\0'}, {'\0'}},
	{"Audio_Channel",               {'\0'}, {'\0'}},
	{"Audio_Simple",                {'\0'}, {'\0'}},
	{"Audio_Volume",                {'\0'}, {'\0'}},
	{"P2p_Uid",                     {'\0'}, {'\0'}},
};/*define a configfile item type table*/

/** MAC 地址 */
common_info_t g_stCommonInfo;

/** 定义Flag标志 */
int g_nUseDefaultFlag = FALSE;

/*******************************************************************************/
/*                                   函数声名区                                      */
/*******************************************************************************/
/** 从flash 读取mac地址 */
int read_mac_addr_from_flash(char *pcMacs);

/** 从flash 读取pb_key */
int read_pb_key_addr_from_flash(char *pcPbKey, int lSizeNe);

/** 初始化flash 的mac pb_key */
int init_flash_common_info();

/** 初始化flash配置信息 */
int init_config_table(char *pszConfigFile);

/** 测试程序 */
void test_config_item();

/** 检测参数 */
int check_flash_paras_state();
/*******************************************************************************/
/*                                   函数实现区                                      */
/*******************************************************************************/
/**
  @brief 初始化flash信息到内存
  @author wfb
  @param[in] 无
  @param[out] 无
  @return 无
  @remark 2013-09-02 增加该注释
  @note
*/
int init_flash_config_parameters(void)
{
    int lRet = EXIT_SUCCESS;
    char acTmpBuf[4] = {0};
    char acCmdBuf[128] = {0};
    int nBufLen = 0;
    FILE *pFile;

    sprintf(acCmdBuf, "touch %s;ralink_init show 2860 > %s", FLASH_CONFIG_PATH, FLASH_CONFIG_PATH);

    pFile = popen(acCmdBuf, "r");

    if (NULL != pFile)
    {
    	char acTmpBuf[128] = {0};

    	fread(acTmpBuf, 1, 128, pFile);
    	pclose(pFile);
    	pFile = NULL;
    }


#if 10
    /* 首先判断flash 参数是否丢失 */
    if (EXIT_SUCCESS != check_flash_paras_state())
    {
    	lRet = EXIT_FAILURE;
    }
#endif
    if (EXIT_SUCCESS != init_flash_common_info())
    {
        lRet = EXIT_FAILURE;
    }

    if (TRUE == g_nUseDefaultFlag)
    {
    	;
//    	if (EXIT_SUCCESS != init_config_table(DEFAULT_CONFIG))
//    	{
//    	    lRet = EXIT_FAILURE;
//    	}
    }
    else
    {
    	if (EXIT_SUCCESS != init_config_table(FLASH_CONFIG_PATH))
    	{
    	    lRet = EXIT_FAILURE;
    	}
    }

    /* 初始化设备类型 */
    get_config_item_value(CONFIG_CAMERA_TYPE,      acTmpBuf,     &nBufLen);
    if (0 == strncmp(acTmpBuf, "M2", 2))
    {
        g_nDeviceType = DEVICE_TYPE_M2;
    }
    else
    {
    	g_nDeviceType = DEVICE_TYPE_M3S;
    }

    /* test */
#if 0
    {
    	test_config_item();
    }
#endif
	return lRet;
}

/**
  @brief 写配置信息写入到flash
  @author wfb
  @param[in] buffer : 待执行的命令
  @param[in] NewValue：对应的参数新值
  @param[in] ItemName：对应的参数名
  @param[out] 无
  @return 无
  @remark 2013-09-02 增加该注释
  @note
*/
int write_config_item_to_flash(char *buffer,char *NewValue,char *ItemName)
{
	FILE *fptr;
	char acTmpBuffer[ITEM_INFO_MAX_LEN] = {0};
	char acReadTmpBuffer[ITEM_INFO_MAX_LEN] = {0};
	int lRetryTime = 2;    ///> 最多尝试写两次
	int lRet = EXIT_FAILURE;
	FILE *pFile;

	if(NULL == buffer || NULL == NewValue || NULL == ItemName) //安全处理
	{
		DEBUGPRINT(DEBUG_INFO, "write_config_item_to_flash: invalid paras\n");
		return lRet;
	}
	while(lRetryTime > 0)
	{
		//system(buffer); /*write value of the item to flash*/
		pFile = popen(buffer, "r");
	    if (NULL != pFile)
	    {
	    	char acTmpBuf[128] = {0};

	    	fread(acTmpBuf, 1, 128, pFile);
	    	pclose(pFile);
	    	pFile = NULL;
	    }
		printf("buf ========= %s\n", buffer);
		sprintf(acTmpBuffer, "nvram_get 2860 %s ", ItemName);
		/*get value of the item from flash*/
		fptr = popen(acTmpBuffer,"r");
		if (fptr == NULL)
		{
			DEBUGPRINT(DEBUG_INFO, "write_config_item_to_flash: fptr INVALID\n");
			printf("write_config_item_to_flash: fptr INVALID\n");
			return lRet;
		}
		/*读取一行不带换行符的数据*/
		fscanf(fptr,"%[^\n]s", acReadTmpBuffer);
		printf("acTmpBuffer = %s\n", acReadTmpBuffer);

		if ((0 == strlen(acReadTmpBuffer)) && (0 == strlen(NewValue)))
		{
			printf("flash strlen = 0, ok\n");
			pclose(fptr);
			return EXIT_SUCCESS;
		}
		/*check if it be write successful*/
		else if(0 == strcmp(acReadTmpBuffer, NewValue))
		{
			printf("flash write success ok\n");
			pclose(fptr);
			return EXIT_SUCCESS;
		}
		else
		{
			printf("flash write not ok\n");
			pclose(fptr);
			lRetryTime -= 1;
		}
	}
	return lRet;
}

/**
  @brief      设置配置信息到内存,但不直接写到flash，当flash读写线程收到更新flash的消息后，会把内存
                中的值写入到flash，与此同时，flash每隔5分钟，会自动将内存上与flash不同步的配置信息写入到flash。
  @author wfb
  @param[in] lItemIndex : 配置项的索引  这个在 common_config.h中被定义
  @param[in] pcItemValue：配置项的值，这里只支持字符型，若是其他非字符型数据，需将其转换成字符型。>
  @param[in] lItemLen：配置项的字符串长度(strlen(pcItemValue))，最大支持ITEM_VALUE_MAX_LEN（255）
  @param[out] 无
  @return EXIT_FAILURE EXIT_SUCCESS
  @remark 2013-09-02 增加该注释
  @note
*/
int set_config_value_to_ram(int lItemIndex,char *pcItemValue, int lItemLen)
{
	int lRet = EXIT_FAILURE;

	do{
	    if( (lItemIndex < 0) || (lItemIndex >= TABLE_ITEM_NUM) )
	    {
	    	DEBUGPRINT(DEBUG_INFO, "iTem = %d,not exist\n", lItemIndex);
		    break;
	    }
	    if ((NULL == pcItemValue) && (0 != lItemLen))
	    {
	    	DEBUGPRINT(DEBUG_INFO, "iTem = %d,pcItemValue invalid\n", lItemIndex);
		    break;
	    }

	    if (lItemIndex > ITEM_VALUE_MAX_LEN)
	    {
		    DEBUGPRINT(DEBUG_INFO, "iTem = %d, lItemLen = %d ,too large \n", lItemIndex, lItemLen);
		    break;
	    }

	    /* 判断字符串和lItem 长度是否一致 */
	    if (NULL == pcItemValue)
	    {
            ;
	    }
	    else if (lItemLen != strlen(pcItemValue))
	    {
	    	DEBUGPRINT(DEBUG_INFO, "lItemIndex = %d, not equal lItemLen = %d, ItemValue = %s \n",
	    			lItemIndex, lItemLen, pcItemValue);
	    }

	    sem_wait(&semConfigfile);/*操作之前尝试获取信号量*/
	    memset(g_stConfigTable[lItemIndex].acNewValue, 0x00, ITEM_VALUE_MAX_LEN);
	    strncpy(g_stConfigTable[lItemIndex].acNewValue, pcItemValue, lItemLen); /*set the pszValue*/
	    g_stConfigTable[lItemIndex].acNewValue[lItemLen] = '\0';
	    sem_post(&semConfigfile);
	    lRet = EXIT_SUCCESS;
	}while(0);

	return lRet;
}


/**
  @brief 调用该函数，将直接把配置信息写入到内存和flash
  @author wfb
  @param[in] lItemIndex : 配置项的索引  这个在 common_config.h中被定义
  @param[in] pcItemValue：配置项的值，这里只支持字符型，若是其他非字符型数据，需将其转换成字符型。
  @param[in] lItemLen：配置项的字符串长度(strlen(pcItemValue))，最大支持ITEM_VALUE_MAX_LEN（255
  @param[out] 无
  @return EXIT_FAILURE EXIT_SUCCESS
  @remark 2013-09-02 增加该注释
  @note
*/
int set_config_value_to_flash(int lItemIndex,char *pcItemValue, int lItemLen)
{
	int  lRet = EXIT_FAILURE;
	char acBuf[ITEM_INFO_MAX_LEN] = {0};

	do{
	    if(lItemIndex<0||lItemIndex>=TABLE_ITEM_NUM)
	    {
	    	DEBUGPRINT(DEBUG_INFO, "iTem = %d,not exist\n", lItemIndex);
		    break;
	    }

	    if ((NULL == pcItemValue) && (0 != lItemLen))
	    {
	    	DEBUGPRINT(DEBUG_INFO, "iTem = %d,pcItemValue invalid but lItemLen = %d\n",lItemIndex, lItemLen);
		    break;
	    }

	    if (lItemLen > ITEM_VALUE_MAX_LEN-1)
	    {
	    	DEBUGPRINT(DEBUG_INFO, "iTem = %d, lItemLen = %d ,too large \n", lItemIndex, lItemLen);
		    break;
	    }

	    /* 判断字符串和lItem 长度是否一致 */
	    if (NULL == pcItemValue)
	    {
	    	;
	    }
	    else if (lItemLen != strlen(pcItemValue))
	    {
	    	DEBUGPRINT(DEBUG_INFO, "lItemIndex = %d, not equal lItemLen = %d, ItemValue = %s \n",
	    			lItemIndex, lItemLen, pcItemValue);
	    }

	    /*操作之前尝试获取信号量*/
	    sem_wait(&semConfigfile);

#if 0
	    /* 如果待写入的和old相同 那么则不需要写入 */
	    if ((NULL == pcItemValue) && (lItemLen == strlen(g_stConfigTable[lItemIndex].acOldValue)))
	    {
    		g_stConfigTable[lItemIndex].acNewValue[lItemLen] = '\0';
	    }
	    else if ((0 == strcmp(pcItemValue, g_stConfigTable[lItemIndex].acOldValue)) &&
	    		(lItemLen == strlen(g_stConfigTable[lItemIndex].acOldValue)))
	    {
            /* old中的值是和flash同步的 即便是old相同 但new有可能不同 这里要重新赋值  */

	    	strncpy(g_stConfigTable[lItemIndex].acNewValue, pcItemValue, lItemLen);


            lRet = EXIT_SUCCESS;
	    }
	    else
#endif
	    {
	    	//如果字符串长度为零 则认为清空
	    	//当设置的同old相同，那么默认则不设置flash
	    	if (0 == lItemLen)
	    	{
	    		if (0 == strlen(g_stConfigTable[lItemIndex].acOldValue))
	    		{
	    			g_stConfigTable[lItemIndex].acNewValue[lItemLen] = '\0';
	    			sem_post(&semConfigfile);
	    			return EXIT_SUCCESS;
	    		}
	    		g_stConfigTable[lItemIndex].acNewValue[lItemLen] = '\0';
	    		g_stConfigTable[lItemIndex].acOldValue[lItemLen] = '\0';
	    		sprintf(acBuf, CLEAR_FLASH_CMD, g_stConfigTable[lItemIndex].acItemName);
	    	}
	    	else
	    	{
	    		/* 若同flash中的相同 那么则不设置 */
	    		if (0 == strcmp(pcItemValue, g_stConfigTable[lItemIndex].acOldValue))
	    		{
	    			memset(g_stConfigTable[lItemIndex].acNewValue, 0x00, ITEM_VALUE_MAX_LEN);
	    			strncpy(g_stConfigTable[lItemIndex].acNewValue, pcItemValue, lItemLen);
	    			g_stConfigTable[lItemIndex].acNewValue[lItemLen] = '\0';
	    			sem_post(&semConfigfile);
	    			return EXIT_SUCCESS;
	    		}
		    	memset(g_stConfigTable[lItemIndex].acNewValue, 0x00, ITEM_VALUE_MAX_LEN);
		    	memset(g_stConfigTable[lItemIndex].acOldValue, 0x00, ITEM_VALUE_MAX_LEN);
		    	strncpy(g_stConfigTable[lItemIndex].acNewValue, pcItemValue, lItemLen);
		    	g_stConfigTable[lItemIndex].acNewValue[lItemLen] = '\0';
		    	strncpy(g_stConfigTable[lItemIndex].acOldValue, pcItemValue, lItemLen);
		    	g_stConfigTable[lItemIndex].acOldValue[lItemLen] = '\0';
	    		sprintf(acBuf, WIRTEFLASHCMD, g_stConfigTable[lItemIndex].acItemName, g_stConfigTable[lItemIndex].acNewValue);
	    	}


	    	lRet = write_config_item_to_flash(acBuf, g_stConfigTable[lItemIndex].acNewValue, g_stConfigTable[lItemIndex].acItemName);
	    }

	    sem_post(&semConfigfile);


	}while(0);

	return lRet;
}

/**
  @brief 获取配置信息
  @author wfb
  @param[in] lItemIndex : 配置项的索引
  @param[in] pcItemValue：配置项的值
  @param[in] lItemLen：配置项的字符串长度
  @param[out] 无
  @return EXIT_FAILURE EXIT_SUCCESS
  @remark 2013-09-02 增加该注释
  @note
*/
int get_config_item_value(int lItemIndex,char *pcItemValue, int *plItemLen)
{
	int lRet = EXIT_FAILURE;

	do{
		/* 参数检查 */
	    if(lItemIndex<0||lItemIndex>=TABLE_ITEM_NUM)
	    {
	    	DEBUGPRINT(DEBUG_INFO, "iTem = %d,not exist\n", lItemIndex);
			 break;
		}

		if (NULL == pcItemValue)
		{
			DEBUGPRINT(DEBUG_INFO, "iTem = %d,pcItemValue invalid\n", lItemIndex);
		    break;
		}

		if (NULL == plItemLen)
		{
			DEBUGPRINT(DEBUG_INFO, "iTem = %d,plItemLen invalid\n", lItemIndex);
		    break;
		}

		/*操作之前尝试获取信号量*/
	    sem_wait(&semConfigfile);
	    strncpy(pcItemValue, g_stConfigTable[lItemIndex].acNewValue, strlen(g_stConfigTable[lItemIndex].acNewValue));
	    *(pcItemValue + strlen(g_stConfigTable[lItemIndex].acNewValue)) = '\0';
	    *plItemLen = strlen(g_stConfigTable[lItemIndex].acNewValue);
	    sem_post(&semConfigfile);

	    lRet = EXIT_SUCCESS;
	}while(0);

	return lRet;
}

/**
  @brief 将ram中的值更新到flash，这个函数只有在flash读写线程中才可以被调用
  @author wfb
  @param[in] 无
  @param[out] 无
  @return EXIT_FAILURE EXIT_SUCCESS
  @remark 2013-09-02 增加该注释
  @note
*/
void updata_ram_config_to_flash()
{
	char acBuf[ITEM_INFO_MAX_LEN] = {0};
	int iNum;

	for(iNum=0;iNum<TABLE_ITEM_NUM;iNum++)
	{
		if(0!=strcmp(g_stConfigTable[iNum].acOldValue, g_stConfigTable[iNum].acNewValue))
		{

			sem_wait(&semConfigfile);

			/* 如果字符串为空  那么则默认为 “” */
			if (0 == strlen(g_stConfigTable[iNum].acNewValue))
			{
				sprintf(acBuf, CLEAR_FLASH_CMD, g_stConfigTable[iNum].acItemName);
			}
			else
			{
				sprintf(acBuf, WIRTEFLASHCMD, g_stConfigTable[iNum].acItemName, g_stConfigTable[iNum].acNewValue);
			}

			if(EXIT_SUCCESS != write_config_item_to_flash(acBuf,g_stConfigTable[iNum].acNewValue, g_stConfigTable[iNum].acItemName))
			{
				printf("write item:%s failure\n",g_stConfigTable[iNum].acItemName);
				sem_post(&semConfigfile);
				continue;
			}
			memcpy(g_stConfigTable[iNum].acOldValue, g_stConfigTable[iNum].acNewValue, sizeof(g_stConfigTable[iNum].acNewValue));
			memset(acBuf, 0x00, ITEM_INFO_MAX_LEN);

			/* 切换出去10ms */
			usleep(10000);
			sem_post(&semConfigfile);
		}
	}
}

/**
  @brief 获取mac地址
  @author wfb
  @param[in] pcMacAddrValue: 获取mac地址指针
  @param[in] plLen : mac地址长度
  @param[out] 无
  @return EXIT_FAILURE EXIT_SUCCESS
  @remark 2013-09-02 增加该注释
  @note
*/
int get_mac_addr_value(char *pcMacAddrValue, int *plLen)
{
	int lRet = EXIT_FAILURE;

	do{
		if (NULL == pcMacAddrValue || NULL == plLen)
		{
			DEBUGPRINT(DEBUG_INFO, " func:get_mac_addr_value,invalid pointer\n");
            break;
		}
		sem_wait(&semConfigfile);
		strncpy(pcMacAddrValue, g_stCommonInfo.acMacAddrBuf, strlen(g_stCommonInfo.acMacAddrBuf));
		*plLen = strlen(g_stCommonInfo.acMacAddrBuf);
		sem_post(&semConfigfile);


	}while(0);

	return lRet;
}

/**
  @brief 设置mac地址
  @author wfb
  @param[in] pcMacAddrValue : mac地址指针
  @param[out] 无
  @return EXIT_FAILURE EXIT_SUCCESS
  @remark 2013-09-02 增加该注释
  @note
*/
int set_mac_addr_value(char *pcMacAddrValue)
{
    int lRet = EXIT_FAILURE;
	char buf[12] = {0};
	FILE *file = NULL;

	if (NULL == pcMacAddrValue)
	{
		DEBUGPRINT(DEBUG_INFO, "func:set_mac_addr_value ,pointer invalid\n");
		return lRet;
	}

	do{
	    sem_wait(&semConfigfile);
	    file = fopen("/dev/mtdblock3", "wb+");

	    if (file == NULL)
	    {
	    	sem_post(&semConfigfile);
	    	DEBUGPRINT(DEBUG_INFO, "func: set_mac_addr_value, file invalid\n");
		    return lRet;
	    }

		/* TODO  检查传入的参数是否正确 */
		utils_astois(buf, pcMacAddrValue, 12);
		if (0 != fseek(file, 40, SEEK_SET))
		{
			DEBUGPRINT(DEBUG_INFO, "func:set_mac_addr_value ,fseek1 failed\n ");
			break;
		}

		if (1 != fwrite(buf, 6, 1, file))
		{
			DEBUGPRINT(DEBUG_INFO, "func:set_mac_addr_value ,fwrite1 failed\n ");
			break;
		}

		if (0 != fseek(file, 4, SEEK_SET))
		{
			DEBUGPRINT(DEBUG_INFO, "func:set_mac_addr_value ,fseek2 failed\n ");
			break;
		}

		if (1 != fwrite(buf + 6, 6, 1, file))
		{
			DEBUGPRINT(DEBUG_INFO, "func:set_mac_addr_value ,fwrite2 failed\n ");
			break;
		}
		fclose(file);

		/* 更新到内存 */
		strncpy(g_stCommonInfo.acMacAddrBuf, pcMacAddrValue, strlen(pcMacAddrValue));
		sem_post(&semConfigfile);

		lRet = EXIT_SUCCESS;
	}while(0);

	if (EXIT_SUCCESS != lRet)
	{
		fclose(file);
		sem_post(&semConfigfile);
	}

	return lRet;
}

/**
  @brief 获取mac地址，该函数从YGZ那里移植过来的
  @author wfb
  @param[in] 无
  @param[out] 无
  @return EXIT_FAILURE EXIT_SUCCESS
  @remark 2013-09-02 增加该注释
  @note
*/
int read_mac_addr_from_flash(char *pcMacs)
{
	int lRet = EXIT_FAILURE;
    char acTmpBuf[64] = {0};

	FILE *file = fopen("/dev/mtdblock3", "rb");

	if (NULL == file)
	{
		return lRet;
	}

	if (fseek(file, 40, SEEK_SET) != 0)
	{
		fclose(file);
		return -1;
	}
	if (fread(acTmpBuf, 6, 1, file) != 1)
	{
		fclose(file);
		return -1;
	}

	if(fseek(file, 4, SEEK_SET) != 0)
	{
		fclose(file);
		return -1;
	}
	if(fread(acTmpBuf + 6, 6, 1, file) != 1)
	{
		fclose(file);
		return -1;
	}

	utils_istoas(pcMacs, acTmpBuf, 12);
//	memmove(macs, buf, 12);
	fclose(file);
	return 0;
}

/**
  @brief 获取pb_key
  @author wfb
  @param[in] pcPbKeyValue： 获取pb_key指针
  @param[in] plLen ：pb_key 长度
  @param[out] 无
  @return EXIT_FAILURE EXIT_SUCCESS
  @remark 2013-09-02 增加该注释
  @note
*/
int get_pb_key_value(char *pcPbKeyValue, int *plLen)
{
	int lRet = EXIT_FAILURE;

	do{
		if (NULL == pcPbKeyValue || NULL == plLen)
		{
			DEBUGPRINT(DEBUG_INFO, " func:get_pb_key_value,invalid pointer\n");
            break;
		}
		sem_wait(&semConfigfile);
		strncpy(pcPbKeyValue, g_stCommonInfo.acPbKey, strlen(g_stCommonInfo.acPbKey));
		*plLen = strlen(g_stCommonInfo.acPbKey);
		sem_post(&semConfigfile);

        lRet = EXIT_SUCCESS;
	}while(0);

	return lRet;
}

/**
  @brief 设置pbkey
  @author wfb
  @param[in] pcMacAddrValue：mac地址指针
  @param[out] 无
  @return EXIT_FAILURE EXIT_SUCCESS
  @remark 2013-09-02 增加该注释
  @note
*/
int set_pb_key_value(const char *pcPbKeyValue, int lPbKeySize)
{
    int lRet = EXIT_FAILURE;
	FILE *file = NULL;

	if (NULL == pcPbKeyValue)
	{
		DEBUGPRINT(DEBUG_INFO, "func:set_pb_key_value ,pointer invalid\n");
		return lRet;
	}

	do{
	    sem_wait(&semConfigfile);
		file = fopen("/dev/mtdblock3", "wb");

		if(file == NULL)
		{
			sem_post(&semConfigfile);
			DEBUGPRINT(DEBUG_INFO, "func:set_pb_key_value ,pointer invalid\n");
			return lRet;
		}
		if(0 != fseek(file, 304, SEEK_SET))
		{
			DEBUGPRINT(DEBUG_INFO, "func:set_pb_key_value ,fseek1 \n");
			break;
		}
		if (1 != fwrite(pcPbKeyValue, lPbKeySize, 1, file))
		{
			DEBUGPRINT(DEBUG_INFO, "func:set_pb_key_value ,fwrite\n");
			break;
		}
		fclose(file);
		/* 更新到内存 */
		strncpy(g_stCommonInfo.acPbKey, pcPbKeyValue,lPbKeySize);
		g_stCommonInfo.acPbKey[lPbKeySize] = '\0';
		sem_post(&semConfigfile);

		lRet = EXIT_SUCCESS;
	}while(0);

	if (EXIT_SUCCESS != lRet)
	{
		fclose(file);
		sem_post(&semConfigfile);
	}

	return lRet;
}

/**
  @brief 获取pb_key，该函数从YGZ那里移植过来的
  @author wfb
  @param[in] pcPbKey：待获取的PB_kEY指针
  @param[in] lSizeNe： 待获取的长度
  @param[out] 无
  @return EXIT_FAILURE EXIT_SUCCESS
  @remark 2013-09-02 增加该注释
  @note
*/
int read_pb_key_addr_from_flash(char *pcPbKey, int lSizeNe)
{
    int lRet = EXIT_FAILURE;

    FILE *file = fopen("/dev/mtdblock3", "rb");

    do{
    	if (NULL == file)
	    {
    		DEBUGPRINT(DEBUG_INFO, "func:read_pb_key_from_flash, open failed\n");
	        break;
    	}

    	if (0 != fseek(file, 304, SEEK_SET))
    	{
    		DEBUGPRINT(DEBUG_INFO, "func:read_pb_key_from_flash, fseek failed\n");
    		fclose(file);
    		break;
    	}

    	if(1 != fread(pcPbKey, lSizeNe, 1, file))
    	{
    		DEBUGPRINT(DEBUG_INFO, "func:read_pb_key_from_flash, fread failed\n");
    		fclose(file);
    		break;
    	}

    	fclose(file);

    	lRet = EXIT_SUCCESS;
    }while(0);

    return lRet;
}

/**
  @brief 检测flash参数状态  若状态不正确则更新
  @author wfb
  @param[in] 无
  @param[out] 无
  @return EXIT_FAILURE EXIT_SUCCESS
  @remark 2013-09-02 增加该注释
  @note
*/
int check_flash_paras_state()
{
    char acCmdBuf[256]  = {0};
    char acReadBuf[256] = {0};
    char acCameraType[16] = {0};
    char acDevType[16] = {0};
    char acVersion[32] = {0};
    FILE *fptr = NULL;
    FILE *fpCameraType = NULL;
    FILE *pFile;
    char *pcRet = NULL;
    int nRet = EXIT_FAILURE;
    int nRenewFlag = FALSE;
    int nFindFlag = FALSE;
    int nMaxTimes = 0;
    int nLen = 0;

    /* 若读到的flash为空 那么则更新(默认为空) */
    strcpy(acCmdBuf, "nvram_get 2860 Conf_Version");
	fptr = popen(acCmdBuf, "r");/*get value of the item from flash*/
	if (fptr == NULL)
	{
		DEBUGPRINT(DEBUG_INFO, "write_config_item_to_flash: fptr INVALID\n")
		return nRet;
	}

	fscanf(fptr,"%[^\n]s", acReadBuf);/*读取一行不带换行符的数据*/

	pclose(fptr);

	/* 比较是否和配置文件的相同 若为空则将 defautl的值写入 */
	if (0 == strlen(acReadBuf))
	{
		DEBUGPRINT(DEBUG_INFO, "THE CONFIG LOST, SO COPY THE DEFAULT TO THE FLASH\n");
        sprintf(acCmdBuf, "ralink_init renew 2860 %s", DEFAULT_CONFIG);
        //system(acCmdBuf);
        pFile = popen(acCmdBuf, "r");
        if (NULL != pFile)
        {
        	char acTmpBuf[128] = {0};

        	fread(acTmpBuf, 1, 128, pFile);
        	pclose(pFile);
        	pFile = NULL;
        }

        DEBUGPRINT(DEBUG_INFO, "THE CONFIG LOST, SO INIT RAM BY DEF CONFIG \n");
        init_config_table(DEFAULT_CONFIG);
        g_nUseDefaultFlag = TRUE;
        return EXIT_SUCCESS;
	}
	else if (0 != strcmp(acReadBuf, CONFIG_INFO_VERSION))
	{
		nRenewFlag = TRUE;
	}

	DEBUGPRINT(DEBUG_INFO, "Conf_Version is : %s+++++++++++++++++++++++++++++++++\n", acReadBuf);




    /* 若不为空 则判断型号是否正确 */
	if (FALSE == nRenewFlag)
	{
		/* 获取flash上 Camera_Type 的值 */
		fpCameraType = popen("nvram_get \"Camera_Type\"", "r");
		if (fpCameraType == NULL)
		{
			DEBUGPRINT(DEBUG_INFO, "get camera type failed\n")
			return nRet;
		}

		fscanf(fpCameraType,"%[^\n]s", acCameraType);/*读取一行不带换行符的数据*/

		DEBUGPRINT(DEBUG_INFO, "acCameraType = %s\n", acCameraType);

		pclose(fpCameraType);

    	fptr=fopen(DEFAULT_CONFIG, "r"); /*open configfile */
		/* 从CONFIG中获取配置文件 */
        do {
        	acReadBuf[0] = '\0';
			fscanf(fptr, "%[^\n]s", acReadBuf);  /*read a line data to buffer*/
			//DEBUGPRINT(DEBUG_INFO, "acReadBuf = %s\n", acReadBuf);
			fgetc(fptr);/*clear '\n'*/
			if (TABLE_ITEM_NUM == nMaxTimes)
			{
				DEBUGPRINT(DEBUG_INFO, "UP TO THE MAX TIMES\n");
				break;
			}
			else
			{
				DEBUGPRINT(DEBUG_INFO, "nMaxTimes = %d, acReadBuf = %s\n", nMaxTimes, acReadBuf);
			}
			if(acReadBuf[0] != '#')
			{
				pcRet = strchr(acReadBuf, '=');
				if(NULL != pcRet)
				{
					*(pcRet++) = '\0';
					//DEBUGPRINT(DEBUG_INFO, "pcRetpcRetpcRetpcRet = %s\n", pcRet);
					if (0 == strcmp(acReadBuf, "Camera_Type"))
					{
						nFindFlag = TRUE;
						DEBUGPRINT(DEBUG_INFO, "acReadBuf = %s\n", acReadBuf);
						if (0 != strcmp(pcRet, acCameraType))
						{
							DEBUGPRINT(DEBUG_INFO, "need renew now \n");
							nRenewFlag = TRUE;
						}
						else
						{
							DEBUGPRINT(DEBUG_INFO, "do not need renew now \n");
						}
						break;
					}
					else
					{
						DEBUGPRINT(DEBUG_INFO, "acReadBuf============ = %s\n", acReadBuf);
					}
				}
			}
			nMaxTimes++;

        }while((FALSE == nFindFlag));

        pclose(fptr);
	}

	if (TRUE == nRenewFlag)
	{
		g_nUseDefaultFlag = TRUE;
		//将默认的值写到flash
		memset(acCmdBuf, 0x00, sizeof(acCmdBuf));
		DEBUGPRINT(DEBUG_INFO, "reset flash param \n");
		gettimeofday(&g_stTimeVal, NULL);
		DEBUGPRINT(DEBUG_INFO, "start time is g_stTimeVal.tv_sec = %ld,g_stTimeVal.tv_usec = %ld\n", g_stTimeVal.tv_sec, g_stTimeVal.tv_usec);

		//(1)将DEFAULT的值刷入
		DEBUGPRINT(DEBUG_INFO, "(1)init_config_table(DEFAULT_CONFIG)\n");
		init_config_table(DEFAULT_CONFIG);

		//(2)将版本号和设备号保存起来
		get_config_item_value(CONFIG_CAMERA_TYPE,  acDevType, &nLen);
		get_config_item_value(CONFIG_CONF_VERSION, acVersion, &nLen);
		DEBUGPRINT(DEBUG_INFO, "(2)CAMERA_TYPE: %s, CONFIG_CONF_VERSION: %s\n", acDevType, acVersion);

		//(3)将FLASH的值同新增加的刷入
		DEBUGPRINT(DEBUG_INFO, "(3)init_config_table(DEFAULT_TMP_CONFIG) \n");
		sprintf(acCmdBuf, "touch %s;ralink_init show 2860 > %s", DEFAULT_TMP_CONFIG, DEFAULT_TMP_CONFIG);
        //system(acCmdBuf);
        pFile = popen(acCmdBuf, "r");
        if (NULL != pFile)
        {
        	char acTmpBuf[128] = {0};

        	fread(acTmpBuf, 1, 128, pFile);
        	pclose(pFile);
        	pFile = NULL;
        }
        memset(acCmdBuf, 0x00, sizeof(acCmdBuf));
        init_config_table(DEFAULT_TMP_CONFIG);

        //(4)将新的config 版本号和类型写入
        DEBUGPRINT(DEBUG_INFO, "(4)WRITE CAMERA_TYPE AND VERSION TO RAM\n");
        set_config_value_to_ram(CONFIG_CAMERA_TYPE,  acDevType, strlen(acDevType));
        set_config_value_to_ram(CONFIG_CONF_VERSION, acVersion, strlen(acVersion));

        //(5)将这些新的值更新到FLASH
        DEBUGPRINT(DEBUG_INFO, "(5)fast_updata_config()\n");
        fast_updata_config_by_flag(TRUE);


		//system("killall nvram_daemon");
		//system("nvram_daemon &;sync");
		//sleep(2);
		gettimeofday(&g_stTimeVal, NULL);
		DEBUGPRINT(DEBUG_INFO, "end time is g_stTimeVal.tv_sec = %ld,g_stTimeVal.tv_usec = %ld\n", g_stTimeVal.tv_sec, g_stTimeVal.tv_usec);
	}

	return nRet;
}

/**
  @brief 初始化 MAC 和 pb_key
  @author wfb
  @param[in] 无
  @param[out] 无
  @return EXIT_FAILURE EXIT_SUCCESS
  @remark 2013-09-02 增加该注释
  @note
*/
int init_flash_common_info()
{
	int lRet = EXIT_SUCCESS;

	/* 初始化mac    */
	if (EXIT_SUCCESS != read_mac_addr_from_flash(g_stCommonInfo.acMacAddrBuf))
	{
		lRet = EXIT_FAILURE;
	}

	/* 初始化pb_key */
	if (EXIT_SUCCESS == read_pb_key_addr_from_flash(g_stCommonInfo.acPbKey, PB_KEY_LEN))
    {
		lRet = EXIT_FAILURE;
	}

	return lRet;
}

/**
  @brief 初始化配置表
  @author wfb
  @param[in] pszConfigFile: 初始化的文件名
  @param[out] 无
  @return EXIT_FAILURE EXIT_SUCCESS
  @remark 2013-09-02 增加该注释
  @note
*/
int init_config_table(char *pszConfigFile)
{
	FILE *	fptr;
	char * cRet;
	char buffer[ITEM_INFO_MAX_LEN] = {0};
	int iCount=0;
	int iNum;

	if(0>sem_init(&semConfigfile,0,1)) /*init semaphore*/
	{
		return -1;
	}
	fptr=fopen(pszConfigFile,"r"); /*open configfile */
	if(NULL==fptr)
	{
		DEBUGPRINT(DEBUG_INFO, "open file  failed,in file :%s,in line:%d\n",__FILE__,__LINE__);
		return -1;
	}
	while((iCount!=TABLE_ITEM_NUM) && (!feof(fptr))) /*init configfile table by read item v*/
	{
		buffer[0]='\0';
		fscanf(fptr,"%[^\n]s",buffer);  /*read a line data to buffer*/
		//DEBUGPRINT(DEBUG_INFO, "get buffer IS %s\n", buffer);
		fgetc(fptr);/*clear '\n'*/
		if(buffer[0]!='#')
		{
			//DEBUGPRINT(DEBUG_INFO, "COME HERE HERE HERE HERE HERE\n");
			cRet=strchr(buffer,'=');
			if(cRet!=NULL)
			{
				*(cRet++)='\0';  /*change '=' to '\0' so that you can get the item,and get the value of the item*/
				//DEBUGPRINT(DEBUG_INFO, "COME HERE HERE HERE HERE HERE  buffer = %s\n", buffer);
				for(iNum=0; iNum<TABLE_ITEM_NUM; iNum++)
				{
					if(0==strcmp(buffer,g_stConfigTable[iNum].acItemName)) /*To list every assignment*/
					{
						memcpy(g_stConfigTable[iNum].acOldValue,cRet,strlen(cRet));/*把cRet指针指向的值赋值给配置表里相应配置项*/
						g_stConfigTable[iNum].acOldValue[strlen(cRet)] = 0;
						memcpy(g_stConfigTable[iNum].acNewValue,cRet,strlen(cRet));
						g_stConfigTable[iNum].acNewValue[strlen(cRet)] = 0;

					#if 10  /*DEBUG信息*/
						DEBUGPRINT(DEBUG_INFO, "%s: %s\n",g_stConfigTable[iNum].acItemName, g_stConfigTable[iNum].acNewValue);
					#endif
						break;
					}
				}
			}
		}
	}

	//关闭文件描述符
	fclose(fptr);

	DEBUGPRINT(DEBUG_INFO,"init_config_table ok .....\n");
	//DEBUGPRINT(DEBUG_INFO,"the number of configitems is:%d,
				//in file:%s,in line:%d\n",iCount,__FILE__,__LINE__);
	return 0;
}

/**
  @brief 测试程序
  @author wfb
  @param[in] 无
  @param[out] 无
  @return EXIT_FAILURE EXIT_SUCCESS
  @remark 2013-09-02 增加该注释
  @note
*/
void test_config_item()
{
	char acBuf[64] = {0};
	int nLen = 0;

	get_config_item_value(CONFIG_CONF_VERSION, acBuf, &nLen);

	DEBUGPRINT(DEBUG_INFO, "CONFIG_CONF_VERSION is %s\n", acBuf);

}

#if 0
int set_str_itme_value(int iTemAdress, char *pszValue)/*用户设置字符串类型的配置项*/
{
	if(-1==SaveUsercfgToTable(iTemAdress,pszValue))
	{
		return -1;
	}else
	{
		return 0;
	}
}



int SetIntItemValue(int iTemAdress, int iValue)/*用户设置整型类型的配置项*/
{
	char buffer[20];
	int iRet = sprintf(buffer, "%d", iValue);/*把整型格式化输入到字符数组里*/
	if(iRet == 0) {
		return -1;
	} else
	{
		return SetStrItemValue(iTemAdress, buffer);
	}
}


int ReadUsercfgFromTable(int iTemAdress, char *pszValue)
{
	if(iTemAdress<0||iTemAdress>=TABLE_ITEM_NUM)
	{
		return -1;  /*Beyond the scope of the config table*/
	}
	if(NULL==pszValue)
	{
		return -1;
	}
	sem_wait(&semConfigfile);
	strncpy(pszValue,g_stConfigTable[iTemAdress].sNewValue,strlen(g_stConfigTable[iTemAdress].sNewValue));
	sem_post(&semConfigfile);
	return 0;
}
int GetStrItemValue(int iTemAdress, char *pszValue)
{
	if(-1==ReadUsercfgFromTable(iTemAdress, pszValue))
	{
		return -1;
	}else
	{
		return 0;
	}
}
int GetIntItemValue(int iTemAdress, int *piValue)
{
	char buffer[200];
	int iRet;
	char *tmp;
	iRet=GetStrItemValue(iTemAdress, buffer);
	if(iRet==0)
	{
		tmp=buffer;
		while('\0'!= *tmp )
		{
			*tmp=(char)tolower(*tmp);
			tmp++;
		}
		if(buffer[0]=='y')
		{
			*piValue = 1;
			return 0;
		}else if(buffer[0]=='n')
		{
			*piValue = 0;
			return 0;
		}else if(sscanf(buffer, "%d",piValue)==1)
		{
			return 0;
		}
	}
	return -1;
}

void FlashConfigFile(const char *pszConfigFile)
{
	if(0 == strcmp(pszConfigFile, USER_CONFIG))
	{
		char buffer[26 + sizeof(USER_CONFIG)];
		sprintf(buffer, "ralink_init renew 2860 %s", USER_CONFIG);
		system(buffer);
	}
}

static inline int FlashGetConfig(char *pszItemName, char *pszValue)
{
	char buffer[200];
	FILE *fPipe;
	if(!fPipe)
	{
		return FAIL;
	}
	sprintf(buffer, "nvram_get 2860 %s", pszItemName);
	fPipe = popen(buffer, "r");
	fscanf(fPipe, "%[^\n]s", pszValue);
    pclose(fPipe);
	return SUCCESS;
}

static inline int FlashSetConfig(char *pszItemName, char *pszValue)
{
	char buffer[200];
	sprintf(buffer, "nvram_set 2860 %s %s", pszItemName, pszValue);
	system(buffer);
	if(SUCCESS != FlashGetConfig(pszItemName, buffer))
		return FAIL;
	if(strncmp(pszValue, buffer, 200))
		return FAIL;
	return SUCCESS;
}

int NewConfigItemStrValue(const char* pszConfigFile, const char* pszItemName, char* pszValue) {
	if(!strcmp(USER_CONFIG, pszConfigFile))
	{
		return FlashSetConfig(pszItemName, pszValue);
	}
	else
	{
		char buffer[200];
		char tmpn[CONFIG_TEMP_PATTERN_SIZE];
		char * eqp;
		FILE * fptr;
		FILE * nfptr;
		int tfd;
		strcpy(tmpn, CONFIG_TEMP_PATTERN);
		tfd = mkstemp(tmpn);
		if(tfd < 0 || (nfptr = fdopen(tfd, "w")) == NULL) {
			return FAIL;
		}
		fptr = fopen(pszConfigFile, "r");
		if(fptr == NULL) {
			fptr = fopen(DEFAULT_CONFIG, "r");
			if(fptr == NULL)
				return FILENOTFOUND;
		}
		// check file for item and copy to new version.
		while (!feof(fptr)) {
			buffer[0] = '\0';
			fscanf(fptr, "%[^\n]s", buffer);
			fgetc(fptr); // clear the \n at the end of the line.
			if(buffer[0] != '#') {
				eqp = strchr(buffer, '=');
				if(eqp == NULL) {
					if(buffer[0] != '\0')
						fprintf(nfptr, "%s\n", buffer);
				} else {
					*(eqp++) = '\0';
					if(strcmp(pszItemName, buffer) == 0) { // abort if item already exists
						fclose(nfptr);
						fclose(fptr);
						remove(tmpn);
						return ITEMISSEXIST;
					}
					fprintf(nfptr, "%s=%s\n", buffer, eqp);
				}
			} else { //copy comment directly
				fprintf(nfptr, "%s\n", buffer);
			}
		}
		fprintf(nfptr, "%s=%s\n", pszItemName, pszValue);
		fclose(nfptr);
		fclose(fptr);
		if(rename(tmpn, pszConfigFile) == 0) {
			FlashConfigFile(pszConfigFile);
			return SUCCESS;
		} else {
			remove(tmpn);
			return FAIL;
		}
	}
}

int NewConfigItemIntValue(const char* pszConfigFile, const char* pszItemName, int iValue) {
	char buffer[12];
	int s = sprintf(buffer, "%d", iValue);
	if(s == 0) {
		return FAIL;
	} else {
		return NewConfigItemStrValue(pszConfigFile, pszItemName, buffer);
	}
}

int GetConfigItemintValue(const char* pszConfigFile, const char* pszItemName, int* piValue) {
	char buffer[100];
	int iStatus = GetConfigItemStrValue(pszConfigFile, pszItemName, buffer);
	if(iStatus == SUCCESS) {
		char * l = buffer;
		while('\0' != *l && sizeof(buffer) < (l - buffer)) {
			//*(l++) = (char)tolower(*l);
			*l = (char)tolower(*l);
			l++;
		}
		if((buffer[0] == 'y') || (strcmp(buffer, "true") == 0) || (strcmp(buffer, "on") == 0)) {
			*piValue = 1;
			return SUCCESS;
		} else if(buffer[0] == 0 || (buffer[0] == 'n') || (strcmp(buffer, "false") == 0) || (strcmp(buffer, "off") == 0)) {
			*piValue = 0;
			return SUCCESS;
		} else if(sscanf(buffer, "%d", piValue) == 1) {
			return SUCCESS;
		} else {
			return FAIL;
		}
	}
	return iStatus;
}

int GetConfigItemStrValue(const char* pszConfigFile, const char* pszItemName, char* pszValue) {
	if(!strcmp(pszConfigFile, USER_CONFIG)) {
		return FlashGetConfig(pszItemName, pszValue);
	}
	char buffer[200];
	char * eqp;
	FILE * fptr;
	int bFound = 0;
	fptr = fopen(pszConfigFile, "r");
	if(fptr == NULL) {
		fptr = fopen(DEFAULT_CONFIG, "r");
		if(fptr == NULL)
			return FILENOTFOUND;
	}
	buffer[0] = '\0';
	while (!(bFound = strcmp(pszItemName, buffer) == 0) && !feof(fptr)) {
		fscanf(fptr, "%[^\n]s", buffer);
		fgetc(fptr); // clear the \n at the end of the line.
		if(buffer[0] != '#') {
			eqp = strchr(buffer, '=');
			if(eqp == NULL)
				eqp = buffer;
			*(eqp++) = '\0';
		}
	}
	fclose(fptr);
	if(bFound) {
		strcpy(pszValue, eqp);
		return SUCCESS;
	} else {
		pszValue[0] = '\0';
		return ITEMNOTFOUND;
	}
}

int SetConfigItemStrValue(const char* pszConfigFile, const char* pszItemName, const char* pszValue) {
	if(!strcmp(pszConfigFile, USER_CONFIG))
	{
		return FlashSetConfig(pszItemName, pszValue);
	}
	char buffer[200];
	char tmpn[CONFIG_TEMP_PATTERN_SIZE];
	char * eqp;
	FILE * fptr;
	FILE * nfptr;
	int found = 0;
	int tfd;
	strcpy(tmpn, CONFIG_TEMP_PATTERN);
	tfd = mkstemp(tmpn);
	if(tfd < 0 || (nfptr = fdopen(tfd, "w")) == NULL) {
		return FAIL;
	}
	fptr = fopen(pszConfigFile, "r");
	if(fptr == NULL) {
		fptr = fopen(DEFAULT_CONFIG, "r");
		if(fptr == NULL)
			return FILENOTFOUND;
	}
	while (!feof(fptr)) {
		buffer[0] = '\0';
		fscanf(fptr, "%[^\n]s", buffer);
		fgetc(fptr); // clear the \n at the end of the line.
		if(buffer[0] != '#') {
			eqp = strchr(buffer, '=');
			if(eqp == NULL) {
				if(buffer[0] != '\0')
					fprintf(nfptr, "%s\n", buffer);
			} else {
				*(eqp++) = '\0';
				if(strcmp(pszItemName, buffer) == 0) {
					fprintf(nfptr, "%s=%s\n", buffer, pszValue);
					found = 1;
				} else {
					fprintf(nfptr, "%s=%s\n", buffer, eqp);
				}
			}
		} else { //copy comment
			fprintf(nfptr, "%s\n", buffer);
		}
	}
	fclose(nfptr);
	fclose(fptr);
	if(found) {
		if(rename(tmpn, pszConfigFile) == 0)
		{
			FlashConfigFile(pszConfigFile);
			return SUCCESS;
		} else {
			//DEBUGPRINT(DEBUG_ERROR, "rename faile:%s, in file:%s line:%d\n",
			//	strerror(errno), __FILE__, __LINE__);
			remove(tmpn);
			return FAIL;
		}
	} else {
		remove(tmpn);
		return ITEMNOTFOUND;
	}
}

int SetConfigItemIntValue(const char* pszConfigFile, const char* pszItemName, int iValue) {
	char buffer[12];
	int s = sprintf(buffer, "%d", iValue);
	if(s == 0) {
		return FAIL;
	} else {
		return SetConfigItemStrValue(pszConfigFile, pszItemName, buffer);
	}
}

// Possible to make this by combining other functions, but that will run more slowly.
int ResetConfigItemValue(const char** pszItemArray, int iArraySize) {
	enum status {
		OMIT,
		MODIFY,
		ATOM,
		DONE
	};
	enum status * status;
	char * value;
	char ** valueArray;
	int i;
	char buffer[200];
	char tmpn[CONFIG_TEMP_PATTERN_SIZE];
	FILE * dfptr;
	FILE * nfptr;
	FILE * cfptr;
	status = (enum status *)malloc(iArraySize * sizeof(enum status));
	if(!status) {
		return FAIL;
	}
	value = (char *)malloc(iArraySize * 100 * sizeof(char));
	if(!value) {
		free(status);
		return FAIL;
	}
	valueArray = (char **)malloc(iArraySize * sizeof(char *));
	if(!valueArray) {
		free(status);
		free(value);
		return FAIL;
	}
	dfptr = fopen(DEFAULT_CONFIG, "r");
	cfptr = fopen(USER_CONFIG, "r");
	{
		int tfd;
		strcpy(tmpn, CONFIG_TEMP_PATTERN);
		tfd = mkstemp(tmpn);
		if(tfd < 0 || (nfptr = fdopen(tfd, "w")) == NULL || dfptr == NULL || cfptr == NULL) {
			free(status);
			free(value);
			free(valueArray);
			if(nfptr) {
				fclose(nfptr);
			}
			if(dfptr) {
				fclose(dfptr);
			}
			if(cfptr) {
				fclose(cfptr);
			}
			return FAIL;
		}
		for(tfd = 0; tfd < iArraySize; tfd++) {
			valueArray[tfd] = value + (100 * tfd);
			status[tfd] = OMIT;
		}
		bzero(value, iArraySize * 100);
	}
	// This section reads the default file and gets the values that should be reset
	while(!feof(dfptr)) {
		buffer[0] = '\0';
		fscanf(dfptr, "%[^\n]s", buffer);
		fgetc(dfptr); // clear the \n at the end of the line.
		if(buffer[0] != '#' && buffer[0] != '\0') {
			char * eqp;
			eqp = strchr(buffer, '=');
			if(eqp != NULL) {
				*(eqp++) = '\0';
			}
			for(i = 0; i < iArraySize; i++) {
				if(eqp == NULL) {
					if(strcmp(buffer, pszItemArray[i]) == 0) {
						status[i] = ATOM;
						break;
					}
				} else {
					if(strcmp(buffer, pszItemArray[i]) == 0) {
						status[i] = MODIFY;
						strcpy(valueArray[i], eqp);
						break;
					}
				}
			}
		}
	}
	// This section reads the current config file and copies the parts that should not be changed, modifies the parts that should be modified.
	while(!feof(cfptr)) {
		buffer[0] = '\0';
		fscanf(cfptr, "%[^\n]s", buffer);
		getc(cfptr);
		if(buffer[0] != '\0') {
			if(buffer[0] == '#') {
				fprintf(nfptr, "%s\n", buffer);
			} else {
				char * eqp = strchr(buffer, '=');
				int f = 0;
				if(eqp != NULL) {
					*(eqp++) = '\0';
				}
				for(i = 0; i < iArraySize; i++) {
					if(strcmp(buffer, pszItemArray[i]) == 0) {
						if(status[i] == ATOM) {
							fprintf(nfptr, "%s\n", pszItemArray[i]);
						} else if(status[i] == MODIFY) {
							fprintf(nfptr, "%s=%s\n", pszItemArray[i], valueArray[i]);
						}
						status[i] = DONE;
						f = 1;
					}
				}
				if(!f) {
					if(eqp) {
						fprintf(nfptr, "%s=%s\n", buffer, eqp);
					} else {
						fprintf(nfptr, "%s\n", buffer);
					}
				}
			}
		}
	}
	for(i = 0; i < iArraySize; i++) {
		if(status[i] == MODIFY) {
			fprintf(nfptr, "%s=%s\n", pszItemArray[i], valueArray[i]);
		} else if(status[i] == ATOM) {
			fprintf(nfptr, "%s\n", pszItemArray[i]);
		}
	}
	free(status);
	free(value);
	free(valueArray);
	fclose(nfptr);
	fclose(dfptr);
	fclose(cfptr);
	if(rename(tmpn, USER_CONFIG) == 0) {
		FlashConfigFile(USER_CONFIG);
		return SUCCESS;
	} else {
		remove(tmpn);
		return FAIL;
	}

}
#endif

/**
  @brief 快速更新根据ram更新
  @author wfb
  @param[in] nParaFlag : TRUE:更新,FALSE: 若ram中的新旧值均相同则不更新，否则 则更新>
  @param[out] 无
  @return EXIT_FAILURE EXIT_SUCCESS
  @remark 2013-09-02 增加该注释
  @note
*/
int fast_updata_config_by_flag(int nParaFlag)
{
    int fd = -1;
    int i = 0;
    int nNeedUpdataFlag = FALSE;
    FILE *pfptr = NULL;
    FILE *pFile = NULL;
    char acCmdBuf[64] = {0};
    char acFilePath[128] = {0};
    char acConfigInfo[512] = {0};

    strncpy(acFilePath, CONFIG_TEMP_PATTERN, CONFIG_TEMP_PATTERN_SIZE);

    DEBUGPRINT(DEBUG_INFO, "COME INTO fast_updata_config_by_flag\\\\\\\\\\\\\\\\\\\\\ \n");
    DEBUGPRINT(DEBUG_INFO, "COME INTO fast_updata_config_by_flag\\\\\\\\\\\\\\\\\\\\\ \n");
    DEBUGPRINT(DEBUG_INFO, "COME INTO fast_updata_config_by_flag\\\\\\\\\\\\\\\\\\\\\ \n");


    sem_wait(&semConfigfile);
    if (-1 == (fd = mkstemp(acFilePath)))
    {
    	return EXIT_FAILURE;
    }

    if (NULL == (pfptr = fdopen(fd, "w")))
    {
        close(fd);
        unlink(acFilePath);
        sem_post(&semConfigfile);
        return EXIT_FAILURE;
    }

    fprintf(pfptr, "%s\n", "#The word of \"Default\" must not be removed");
    fprintf(pfptr, "%s\n", "Default");

    for (i=0; i < TABLE_ITEM_NUM; i++)
    {
     	if (strcmp(g_stConfigTable[i].acOldValue, g_stConfigTable[i].acNewValue))
    	{
    		nNeedUpdataFlag = TRUE;
    		strcpy(g_stConfigTable[i].acOldValue, g_stConfigTable[i].acNewValue);
    	}

    	sprintf(acConfigInfo, "%s=%s", g_stConfigTable[i].acItemName, g_stConfigTable[i].acNewValue);
    	fprintf(pfptr, "%s\n", acConfigInfo);
    }

    fclose(pfptr);
    close(fd);

    if ((TRUE == nNeedUpdataFlag) || (TRUE == nParaFlag))
    {
    	sprintf(acCmdBuf, "ralink_init renew 2860 %s", "/etc/Wireless/RT2860/tmpcfg*");
		//system(acCmdBuf);
        pFile = popen(acCmdBuf, "r");
        if (NULL != pFile)
        {
        	char acTmpBuf[128] = {0};

        	fread(acTmpBuf, 1, 128, pFile);
        	pclose(pFile);
        	pFile = NULL;
        }
    }

    unlink(acFilePath);
    sem_post(&semConfigfile);
    DEBUGPRINT(DEBUG_INFO, "OK OK fast_updata_config_by_flag\\\\\\\\\\\\\\\\\\\\\ \n");
    DEBUGPRINT(DEBUG_INFO, "OK OK fast_updata_config_by_flag\\\\\\\\\\\\\\\\\\\\\ \n");
    DEBUGPRINT(DEBUG_INFO, "OK OK fast_updata_config_by_flag\\\\\\\\\\\\\\\\\\\\\ \n");

    return EXIT_SUCCESS;
}
