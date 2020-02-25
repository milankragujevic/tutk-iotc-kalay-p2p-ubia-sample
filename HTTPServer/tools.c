/*
 * tools.c
 *
 *  Created on: Apr 9, 2013
 *      Author: root
 */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include "tools.h"
#include "encryption.h"

int ReadFile2Binary(char **pFileBuffer, const char *pFileName);
int ChangeBase64ToURL(char *pCamEnrIDToURLEncode, const char *pCamEncryptionID);
int URLDecode(char *str, int len);
char *URLEncode(char const *s, int len, int *new_length);
int ClearCRLF(char *pStrText);
int CreatIPUEncryptionID(char *pCamEncryptionID, const char *pRandom,char * strkeyN,char *strkeyE);
int SetStableFlag(void);
/* get the enr radon number */
int CreatIPUEncryptionID(char *pCamEncryptionID, const char *pRandom,char * strkeyN,char *strkeyE)
{
    char strRSACipher[512];

    if((NULL==pCamEncryptionID) || (NULL==pRandom))
    {
        return -1;
    }

    memset(strRSACipher, '\0', sizeof(strRSACipher));
    RSAPublicEncryption_IPU( strkeyN, strkeyE,strRSACipher, pRandom, 1);
    Base64Encode(pCamEncryptionID, strRSACipher, 128);

    return 0;
}

/**************************************************************************************************************************
  清除回车换行符
 *************************************************************************************************************************/
int ClearCRLF(char *pStrText)
{
    int i = 0;
    int j = 0;
    int n = 0;
    int nLength = 0;

    if(NULL == pStrText)
    {
        return -1;
    }


	nLength = strlen(pStrText);

    for(i=0; i<=nLength-n; i++)
    {
        if((pStrText[i]=='\n') || (pStrText[i]=='\r'))
        {
            for(j=i; j<=nLength-1-n; j++)
            {
                pStrText[j] = pStrText[j+1];
            }
            n++;
        }
    }

    return 0;
}

int ChangeBase64ToURL(char *pCamEnrIDToURLEncode, const char *pCamEncryptionID)
{
    int nLeng=0,length=0;
    char *pURLEncode;

    if((NULL==pCamEnrIDToURLEncode) || (NULL==pCamEncryptionID))
    {
        return -1;
    }

    pURLEncode = URLEncode(pCamEncryptionID, strlen(pCamEncryptionID), &nLeng);
    memcpy(pCamEnrIDToURLEncode, pURLEncode, strlen(pURLEncode));
    length = strlen(pURLEncode);
	free(pURLEncode);

    return length;
}


/**************************************************************************************************************************
 * change Base64 code to URLcode
 *************************************************************************************************************************/
char *URLEncode(char const *s, int len, int *new_length)
{
    char *start;
    char *to;
	char c;
    char const *from;
    char const *end;

	from = s;
    end = s + len;
    start = to = (char *) malloc(3 * len + 1);

    char hexchars[] = "0123456789abcdef";

    while (from < end)
	{
        c = *from++;

        if (c == ' ')
		{
            *to++ = '+';
        }
		else if ((c < '0' && c != '-' && c != '.')
                   ||(c < 'A' && c > '9')
                   ||(c > 'Z' && c < 'a' && c != '_')
                   ||(c > 'z'))
 		{
            to[0] = '%';
            to[1] = hexchars[c >> 4];
            to[2] = hexchars[c & 15];
            to += 3;
        }
		else
		{
            *to++ = c;
        }
    }

    *to = 0;
    if (new_length)
	{
        *new_length = to - start;
    }

    return (char *)start;
}


int URLDecode(char *str, int len)
{
    char *dest = str;
    char *data = str;

    int value;
    int c;

    while (len--)
	{
        if (*data == '+')
		{
        	*dest = ' ';
        }
        else if (*data == '%' && len >= 2 && isxdigit((int) *(data + 1))
                 && isxdigit((int) *(data + 2)))
        {

            c = ((char *)(data+1))[0];
            if (isupper(c))
            {
            	c = tolower(c);
            }
            value = (c >= '0' && c <= '9' ? c - '0' : c - 'a' + 10) * 16;
            c = ((char *)(data+1))[1];
            if (isupper(c))
            {
            	c = tolower(c);
			}
            value += c >= '0' && c <= '9' ? c - '0' : c - 'a' + 10;

            *dest = (char)value ;
            data += 2;
            len -= 2;
        }
		else
		{
            *dest = *data;
        }
        data++;
        dest++;
    }

    *dest = '\0';

    return (dest - str);
}
int DecodeEncryptionID(char *EncryptionID){
	int nRet = 0;

	if(EncryptionID==NULL){
		return -1;
	}

	char random[9];
	memset(random,0,sizeof(random));
	printf("EncryptionID-->EncryptionID:%s\n",EncryptionID);
	if(-1 == RSAPrivateDecryption(random,EncryptionID,3)){

	}else{
		printf("EncryptionID-->random:%s\n",random);
	}


	return nRet;
}

/* read file */
int ReadFile(char **pFileBuffer, const char *pFileName)
{
    int nRetLeng = 0;
    FILE *pFile = NULL;
    int nFileLength = 0;
    int nReadLeng = 0;
    char *pBuffer = NULL;
    char *pBase = NULL;
    char *pURLEncode = NULL;

    if ((NULL==pFileName) ||(NULL==pFileName))
    {
        return -2;
    }

    pFile = fopen(pFileName, "rb");
    if(NULL == pFile)
    {
        return -3;
    }

    fseek(pFile, 0, SEEK_END);
    nFileLength = ftell(pFile);
    rewind(pFile);
    printf("*********nFileLength:%d**************\n",nFileLength);
    //read file to the buffer
    pBuffer = (char *)malloc(nFileLength+1);
    if (NULL == pBuffer)
    {
        fclose(pFile);
        pFile = NULL;
        return -4;
    }
    memset(pBuffer, 0, (nFileLength+1));
    nReadLeng = fread(pBuffer, 1, nFileLength, pFile); printf("*********nReadLeng:%d**************\n",nReadLeng);
    if(nReadLeng <= 0)
    {
        free(pBuffer);
        pBuffer = NULL;
        fclose(pFile);
        pFile = NULL;
        return -1;
    }
    fclose(pFile);
    pFile = NULL;

    //create base64 code
    pBase = (char *)malloc(2*nFileLength);
    if (NULL == pBase)
    {
        free(pBuffer);
        pBuffer = NULL;
        return -5;
    }
    memset(pBase, 0, (2*nFileLength));
    Base64Encode(pBase, pBuffer, nFileLength);

    free(pBuffer);
    pBuffer = NULL;

    //create url code
    pURLEncode = (unsigned char *)malloc(2*nFileLength+512);
    if (NULL == pURLEncode)
    {
        free(pBase);
        pBase = NULL;
        return -5;
    }
    memset(pURLEncode, 0, (2*nFileLength+512));
    ChangeBase64ToURL(pURLEncode, pBase);

    *pFileBuffer = pURLEncode;

    pURLEncode = NULL;
    free(pBase);
    pBase = NULL;

    nRetLeng = strlen(*pFileBuffer);

    return nRetLeng;
}

/* read file */
int ReadFile2Binary(char **pFileBuffer, const char *pFileName)
{
    int nRetLeng = -1;
    FILE *pFile = NULL;
    int nFileLength = 0;
    int nReadLeng = 0;
    char *pBuffer = NULL;
    char *pBase = NULL;
//    char *pURLEncode = NULL;

    if ((NULL==pFileName) ||(NULL==pFileName))
    {
        return -2;
    }

    pFile = fopen(pFileName, "rb");
    if(NULL == pFile)
    {
        return -3;
    }

    fseek(pFile, 0, SEEK_END);
    nFileLength = ftell(pFile);
    rewind(pFile);

    //read file to the buffer
    pBuffer = (char *)malloc(nFileLength+1);
    if (NULL == pBuffer)
    {
        fclose(pFile);
        pFile = NULL;
        return -4;
    }
    memset(pBuffer, 0, (nFileLength+1));
    nReadLeng = fread(pBuffer, 1, nFileLength, pFile);

    if(nReadLeng <= 0)
    {
        free(pBuffer);
        pBuffer = NULL;
        fclose(pFile);
        pFile = NULL;
        return 0;
    }
    fclose(pFile);
    pFile = NULL;
//////////////////////////////////////////////////////////
    //create base64 code
    pBase = (char *)malloc(2*nFileLength);
    if (NULL == pBase)
    {
        free(pBuffer);
        pBuffer = NULL;
        return 0;
    }

    memset(pBase, 0, (2*nFileLength));
    Base64Encode(pBase, pBuffer, nFileLength);

    free(pBuffer);
    pBuffer = NULL;
#if 0
    //create url code
    pURLEncode = (unsigned char *)malloc(2*nFileLength+512);
    if (NULL == pURLEncode)
    {
        free(pBase);
        pBase = NULL;
        return -5;
    }
    memset(pURLEncode, 0, (2*nFileLength+512));
    ChangeBase64ToURL(pURLEncode, pBase);

    *pFileBuffer = pURLEncode;

    pURLEncode = NULL;
    free(pBase);
    pBase = NULL;
#endif
    *pFileBuffer = pBase;
//    nRetLeng = strlen(pBuffer);

    return strlen(pBase);
}

int ReadStable(unsigned char *pImage1Stable)
{
    if (NULL == pImage1Stable)
    {
        return -1;
    }

    FILE *fp;
    fp=popen( "nvram_get uboot Image1Stable","r");
    fgets(pImage1Stable,sizeof(pImage1Stable),fp);
   // printf("%s",pImage1Stable);

    if (strlen(pImage1Stable) <= 0)
    {
    	pclose(fp);
        return -1;
    }
    if (strlen(pImage1Stable)>0)
    {
        if (pImage1Stable[strlen(pImage1Stable)-1] == '\n')
        {
            pImage1Stable[strlen(pImage1Stable)-1] = '\0';
        }
    }

    pclose(fp);

    return 0;
}

int SetStableFlag(void)
{
	char buf[2] = {'\0'};
	FILE *pFile = NULL;

	FILE *file = popen( "nvram_get uboot Image1Stable","r");
	if(file != NULL) {
		fgets(buf,2,file);
		if(buf[0] == '1') {

		}
		else {
			//system("cp /dev/mtdblock4 /dev/mtdblock5");
			//system("nvram_set uboot Image1Try 0");
			//system("nvram_set uboot Image1Stable 1");
			pFile = popen("cp /dev/mtdblock4 /dev/mtdblock5", "r");
			if (NULL != pFile)
			{
				char acTmpBuf[128] = {0};

				fread(acTmpBuf, 1, 128, pFile);
				pclose(pFile);
				pFile = NULL;
			}

			pFile = popen("nvram_set uboot Image1Try 0", "r");
			if (NULL != pFile)
			{
				char acTmpBuf[128] = {0};

				fread(acTmpBuf, 1, 128, pFile);
				pclose(pFile);
				pFile = NULL;
			}

			pFile = popen("nvram_set uboot Image1Stable 1", "r");
			if (NULL != pFile)
			{
				char acTmpBuf[128] = {0};

				fread(acTmpBuf, 1, 128, pFile);
				pclose(pFile);
				pFile = NULL;
			}
		}
	}
	pclose(file);
	return 0;
}


