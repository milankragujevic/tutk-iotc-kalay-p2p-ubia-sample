#include <openssl/ssl.h>
#include <openssl/err.h>
#include <openssl/md5.h>

#include <stdio.h>
#include <string.h>
#include <stdlib.h>

//#include "../inc/utility.h"
//#include "../../common/inc/common.h"
//#include "configfile.h"


#define N   "E10BEA491EBC6B65DE0E2B32366FCA8A512BECC168F7F5829EF260473AC2757A7EA432D8371E7A2AF0ACEFEAC8A128C7978C45FC934D19EB6DCA2634CC98B42D4D45AAEB294ACEBFF4BB9438C4BD76A3EADB4BB7AE18BC99A95B0AA16B72D3C4B52F10B8A1C5EB60969161540865AECFC16A87285FC520528521D6BE2444E11F"
#define	E	"010001"

#define	P	"F84E42ED8695B7A10B1D7DC788AECCC3FAC0FCBDFA93525AD72FCFD704E35E8E690299550C7BA58794CCC01EBA4289BFA36F42B89B39EC2CECD8220935B26EEF"
#define	Q	"E8052538F4B0DCAFE1BD97C6757391FA003EF434914CF34F2AB038827565F47D75997BB3123023BF053FF166E55303A0AFAF7163293ED4804E276FB573ECB0D1"
#define	DP	"ADF5565A4641B9C66F9D17B3A504A19C639EB4F2FC0C1E545A11BB10AEF2041ACD62EEBD70E367529762E2EE241BDD998F0CB1B7D7B83AC8369E2D2A3A9E69AD"
#define	DQ	"53782E68EC16887E39CEF4403056D06849185CD06089776ABEA7C0DCA61174C081C322AE4C57C7345C5621A96BCCCE9C4B37E9A9CBD7CFA90CF4C10A5D570451"
#define	IQ	"77176363FC7A63866D9CC8E8CCF69C43B05771DD4346333591E517BCAAB55B6F932DC08ABE452875A814DBB5A4E1B04F1CA5971302C213F412D058DD21C73EF9"
#define D "588D93B709FD22558737141042136323622562980871E98D6875FF24881E94938AD9999832B82F624E29FE8DE83C620B87BA8E9F8066CA58356F61F14CC099C4ACADF74FE5198CD4BBC8BE8EFC6F72CBEBC1E8C9C1596E9386D7846242A70E1BAD7350CE7808047B2D3E18DFA3A6A24172A92B76BE7522FDAB5A28C44EC28281"

#define PRIVATE_KEY	    "privatekey.pem"
#define PUBLIC_KEY      "publickey.pem"

#define PRIVATE_TYPE     1
#define PUBLIC_TYPE      2


static char base64Alphabet[]= 
{
'A','B','C','D','E','F','G','H','I','J','K','L','M','N','O','P', 
'Q','R','S','T','U','V','W','X','Y','Z','a','b','c','d','e','f', 
'g','h','i','j','k','l','m','n','o','p','q','r','s','t','u','v', 
'w','x','y','z','0','1','2','3','4','5','6','7','8','9','+','/','='
};


static int IndexInAlphabet(char c)
{
    int i = 0;

    for (i=0; i<65; ++i)
    {    
        if (c == base64Alphabet[i])
        {   
            return i;
        }
    }

    return -1;
}


unsigned char* base64Decode(const char* source, const int sourceLength, int *nLeng)
{
    unsigned char* result = NULL;
    unsigned int resultLength = sourceLength/4*3;
    unsigned int i = 0;
    unsigned int j = 0;

    if(source[sourceLength-2] == '=')
    {
        *nLeng = resultLength - 2;
    }
    else if (source[sourceLength-1] == '=')
    {
        *nLeng = resultLength - 1;
    }
    else
    {
        *nLeng = resultLength;
    }

    result = (unsigned char*)malloc(resultLength + 1);
    if(NULL == result)
    {
        return NULL;
    }
    memset(result, 0, resultLength+1);

    int counts = sourceLength/4;
 
    for (i=0; i<counts; ++i)
    {
        int sourceIndex = i*4;
        int resultIndex = i*3;

        int buffer[4];
        int padding = 0;

        for (j=0; j<4; ++j)
        {
            buffer[j] = IndexInAlphabet(source[sourceIndex+j]);
        }
 
        if (i == counts-1)
        {
            for (j=0; j<4; ++j)
            {
                if (buffer[j] == 0x40)
                {
                    padding++;
                }
            }
        }

        result[resultIndex] = ((buffer[0]&0x3F)<<2 | (buffer[1]&0x30)>>4);
        if (padding == 2)
        {
            break;
        }

        result[resultIndex+1] = ((buffer[1]&0x0F)<<4 | (buffer[2]&0x3C)>>2);
        if (padding == 1)
        {
           break;
        }

        result[resultIndex+2] = ((buffer[2]&0x03)<<6 | (buffer[3]&0x3F));
    }

    return result;
}

static int Baser64ToHex(char* pHexText, char *pBase64, int nBaseLeng)
{
	int i = 0;
	int nLeng = 0;
	char strTemp[3];
    unsigned char* pResult = NULL;

    pResult = base64Decode(pBase64, nBaseLeng, &nLeng);

	for(i=0; i < nLeng; i++)
	{
		memset(strTemp, 0, sizeof(strTemp));
		sprintf(strTemp, "%02X", pResult[i]);
		strcat(pHexText, strTemp);
	}
	
    free(pResult);

    return 0;
}


static int CreatePrivateRSA(RSA *pRSA)
{	
    BIGNUM *bnn = NULL;	
    BIGNUM *bne = NULL;	
    BIGNUM *bnd = NULL;	
    BIGNUM *bnp = NULL;	
    BIGNUM *bnq = NULL;	
    BIGNUM *bndp = NULL;	
    BIGNUM *bndq = NULL;	
    BIGNUM *bniq = NULL;	

    if(NULL == pRSA)
    {
        //DEBUGPRINT(DEBUG_ERROR, "Pointer is NULL! @:%s @:%d\n", __FILE__, __LINE__);
        return -1;
    }

    bnn = BN_new();	
    bne = BN_new();	
    bnd = BN_new();	
    bnp = BN_new();	
    bnq = BN_new();	
    bndp = BN_new();	
    bndq = BN_new();	
    bniq = BN_new();	

    BN_hex2bn(&bnn, N);
    BN_hex2bn(&bne, E);
    BN_hex2bn(&bnd, D);
    BN_hex2bn(&bnp, P);
    BN_hex2bn(&bnq, Q);
    BN_hex2bn(&bndp, DP);
    BN_hex2bn(&bndq, DQ);
    BN_hex2bn(&bniq, IQ);

    pRSA->n = bnn;	
    pRSA->e = bne;	
    pRSA->d = bnd;	
    pRSA->p = bnp;	
    pRSA->q = bnq;	
    pRSA->dmp1 = bndp;	
    pRSA->dmq1 = bndq;	
    pRSA->iqmp = bniq;	

    //RSA_print_fp(stdout, pRSA, 11);

    return 0;
}	

static int CreatePublicRSA(RSA *pRSA)
{	
    BIGNUM *bnn = NULL;	
    BIGNUM *bne = NULL;	

    if(NULL == pRSA)
    {
        //DEBUGPRINT(DEBUG_ERROR, "Pointer is NULL! @:%s @:%d\n", __FILE__, __LINE__);
        return -1;
    }

    bnn = BN_new();	
    bne = BN_new();	

    BN_hex2bn(&bnn, N);	
    BN_hex2bn(&bne, E);			

    pRSA->n = bnn;	
    pRSA->e = bne;	
	
    //RSA_print_fp(stdout, pRSA, 11);

    return 0;
}	

/////////////////////////////////////////////////////////

static int CreatePublicRSAKey(RSA *pRSA, char *pKeyN, char *pKeyE)
{	
    BIGNUM *bnn = NULL;	
    BIGNUM *bne = NULL;	

    if ((NULL==pRSA) || (NULL==pKeyN) || (NULL==pKeyE))
    {
        //DEBUGPRINT(DEBUG_ERROR, "Pointer is NULL! @:%s @:%d\n", __FILE__, __LINE__);
        return -1;
    }

    bnn = BN_new();	
    bne = BN_new();	

    BN_hex2bn(&bnn, pKeyN);	
    BN_hex2bn(&bne, pKeyE);			

    pRSA->n = bnn;	
    pRSA->e = bne;	
	
    //RSA_print_fp(stdout, pRSA, 11);

    return 0;
}	
/////////////////////////////////////////////////////////

static int CreateRSAKey(RSA **pRSA, const char *pPemFileName, const int nKeyType)
{	
    FILE *pPemFile =  NULL;

    if ((NULL==pRSA) || (NULL==pPemFileName))
    {
        return -1;
    }

    pPemFile = fopen(pPemFileName, "r");
    if(pPemFile == NULL)
    {    
        //DEBUGPRINT(DEBUG_ERROR, "read pem error! @:%s @:%d\n", __FILE__, __LINE__);
        return -2;
    }

    if(nKeyType == PRIVATE_TYPE)
    {
        *pRSA = PEM_read_RSAPrivateKey(pPemFile, NULL, NULL, NULL);
    }
    else if (nKeyType == PUBLIC_TYPE)
    {
        *pRSA = PEM_read_RSAPublicKey(pPemFile, NULL, NULL, NULL);
    }

    if(NULL == (*pRSA))
    {
        //DEBUGPRINT(DEBUG_ERROR, "create rsa error! @:%s @:%d\n", __FILE__, __LINE__);
        return -3;
    }

    fclose(pPemFile);

    //RSA_print_fp(stdout, *pRSA, 11);

    return 0;
}

/* RSA 公钥加密 */
int RSAPublicEncryption_IPU(char *STRKEYN,char *STRKEYE,char *pCipherText, const char *pClearText, const int nPandding)
{
    int nFlen = 0;
    int nRet = 0;
    RSA *pRSA = NULL;

    char strKeyN[172+1];
    char strKeyE[4+1];
//    char strKeyN[] = "xL+8cIRY5Vp/O79pegXS+PIdeTceifOuIVBIjg/RtZde8qYB95i5VuJx1vp1mDYGtRtII224RqZnBLkTP8LWRZlbWckp9uNrs1IUUd2rg51Tz3rREErQhYlND1f69scqsM8sjPFrK6zQpVlIh9iMDMmgALO9YvebDaYVpHLkgU8=";
//    char strKeyE[] = "AQAB";
    memset(strKeyN, '\0', sizeof(strKeyN));
    memcpy(strKeyN,STRKEYN,sizeof(strKeyN));
	memset(strKeyE, '\0', sizeof(strKeyE));
    memcpy(strKeyE,STRKEYE,sizeof(strKeyE));

    char strHexKeyN[256+1];
    char strHexKeyE[6+1];

//    memset(strKeyN, '\0', sizeof(strKeyN));
    //ReadKeyN(strKeyN);
    //GetRSAKeyN(strKeyN, (sizeof(strKeyN)-1)*2);
#if 0
    ReadKeyNN(strKeyN, (sizeof(strKeyN)-1));
#endif

    //printf("KeyN:\n%s\n", strKeyN);

//    memset(strKeyE, '\0', sizeof(strKeyE));
    //ReadKeyE(strKeyE);
    //GetRSAKeyE(strKeyE, (sizeof(strKeyE)-1)*2);
#if 0
    ReadKeyEE(strKeyE, (sizeof(strKeyE)-1));
#endif

    //printf("KeyE:\n%s\n", strKeyE);

    memset(strHexKeyN, '\0', sizeof(strHexKeyN));
    Baser64ToHex(strHexKeyN, strKeyN, strlen(strKeyN));


    memset(strHexKeyE, '\0', sizeof(strHexKeyE));
    Baser64ToHex(strHexKeyE, strKeyE, strlen(strKeyE));

    pRSA = RSA_new();
    CreatePublicRSAKey(pRSA, strHexKeyN, strHexKeyE);

    nFlen = RSA_size(pRSA);
    switch(nPandding)
    {
       case RSA_PKCS1_PADDING:
            nFlen -= 11;
            break;
       case RSA_SSLV23_PADDING:
            nFlen -= 11;
            break;
       case RSA_X931_PADDING:
            nFlen -= 2;
            break;
       case RSA_NO_PADDING:
//            nFlen = nFlen;
            break;
       case RSA_PKCS1_OAEP_PADDING:
            nFlen = nFlen - 2 * SHA_DIGEST_LENGTH - 2;
            break;
       default :
			//DEBUGPRINT(DEBUG_ERROR, "rsa not surport! @:%s @:%d\n", __FILE__, __LINE__);
			return -1;
            break;
    }
//    printf("<>--------------nFlen:%d----------------<>",nFlen);
    nRet = RSA_public_encrypt(8, pClearText, pCipherText, pRSA, nPandding);
    if(nRet <= 0)
    {
       // DEBUGPRINT(DEBUG_ERROR, "RSA_public_encrypt err! @:%s @:%d\n", __FILE__, __LINE__);
        return -1;
    }

    RSA_free(pRSA);

    return nRet;
}
/* RSA 公钥加密 */
int RSAPublicEncryption(/*unsigned */char *pCipherText, const /*unsigned */char *pClearText, const int nPandding)
{
    int nFlen = 0;
    int nRet = 0;
    RSA *pRSA = NULL;

//    char strKeyN[172+1];
//    char strKeyE[4+1];
    char strKeyN[] = "xL+8cIRY5Vp/O79pegXS+PIdeTceifOuIVBIjg/RtZde8qYB95i5VuJx1vp1mDYGtRtII224RqZnBLkTP8LWRZlbWckp9uNrs1IUUd2rg51Tz3rREErQhYlND1f69scqsM8sjPFrK6zQpVlIh9iMDMmgALO9YvebDaYVpHLkgU8=";
    char strKeyE[] = "AQAB";
    char strHexKeyN[256+1];
    char strHexKeyE[6+1];

//    memset(strKeyN, '\0', sizeof(strKeyN));
    //ReadKeyN(strKeyN); 
    //GetRSAKeyN(strKeyN, (sizeof(strKeyN)-1)*2);
#if 0
    ReadKeyNN(strKeyN, (sizeof(strKeyN)-1));
#endif

    printf("KeyN:\n%s\n", strKeyN);

//    memset(strKeyE, '\0', sizeof(strKeyE));
    //ReadKeyE(strKeyE); 
    //GetRSAKeyE(strKeyE, (sizeof(strKeyE)-1)*2);
#if 0
    ReadKeyEE(strKeyE, (sizeof(strKeyE)-1));
#endif

    printf("KeyE:\n%s\n", strKeyE);

    memset(strHexKeyN, '\0', sizeof(strHexKeyN));
    Baser64ToHex(strHexKeyN, strKeyN, strlen(strKeyN));


    memset(strHexKeyE, '\0', sizeof(strHexKeyE));
    Baser64ToHex(strHexKeyE, strKeyE, strlen(strKeyE));

    pRSA = RSA_new();
    CreatePublicRSAKey(pRSA, strHexKeyN, strHexKeyE);

    nFlen = RSA_size(pRSA);
    switch(nPandding)
    {
       case RSA_PKCS1_PADDING:
            nFlen -= 11;
            break;
       case RSA_SSLV23_PADDING:
            nFlen -= 11;
            break;
       case RSA_X931_PADDING:
            nFlen -= 2;
            break;
       case RSA_NO_PADDING:
//            nFlen = nFlen;
            break;
       case RSA_PKCS1_OAEP_PADDING:
            nFlen = nFlen - 2 * SHA_DIGEST_LENGTH - 2;
            break;
       default : 
			//DEBUGPRINT(DEBUG_ERROR, "rsa not surport! @:%s @:%d\n", __FILE__, __LINE__);
			return -1;
            break;
    }

    nRet = RSA_public_encrypt(nFlen, pClearText, pCipherText, pRSA, nPandding);
    if(nRet <= 0)
    {
       // DEBUGPRINT(DEBUG_ERROR, "RSA_public_encrypt err! @:%s @:%d\n", __FILE__, __LINE__);
        return -1;
    }

    RSA_free(pRSA);

    return nRet;
}

/* RSA 私钥解密 */
int  RSAPrivateDecryption(unsigned char *pClearText, const unsigned char *pCipherText, const int nPandding)
{
    int nFlen = 0;
    int nRet = 0;
    RSA *pRSA = NULL;

    pRSA = RSA_new();
    CreatePrivateRSA(pRSA);

    nFlen = RSA_size(pRSA);
    switch(nPandding)
    {
       case RSA_PKCS1_PADDING:
            nFlen -= 11;
            break;
       case RSA_SSLV23_PADDING:
            nFlen -= 11;
            break;
       case RSA_X931_PADDING:
            nFlen -= 2;
            break;
       case RSA_NO_PADDING:
//            nFlen = nFlen;
            break;
       case RSA_PKCS1_OAEP_PADDING:
            nFlen = nFlen - 2 * SHA_DIGEST_LENGTH - 2;
            break;
       default : 
			//DEBUGPRINT(DEBUG_ERROR, "rsa not surport! @:%s @:%d\n", __FILE__, __LINE__);
			return -1;
            break;
    }

    nRet = RSA_private_decrypt(nFlen, pCipherText, pClearText, pRSA, nPandding);
    if(nRet <= 0)
    {
        //DEBUGPRINT(DEBUG_ERROR, "RSA_private_decrypt err! @:%s @:%d\n", __FILE__, __LINE__);
        return -1;
    }

    RSA_free(pRSA);

    return nRet;
}

/* BASE64 编码 */
void Base64Encode(unsigned char *pOut, const unsigned char *pIn, const long nSize)
{   
    EVP_ENCODE_CTX ctxEncode; 
	int	nOut = 0;
    int nTotal = 0;
	
	EVP_EncodeInit(&ctxEncode);
	EVP_EncodeUpdate(&ctxEncode, pOut, &nOut, pIn, nSize);
    nTotal += nOut;
	EVP_EncodeFinal(&ctxEncode, pOut+nTotal, &nOut);
}

/* BASE64 解码 */
int Base64Decode(unsigned char *pOut, const unsigned char *pIn, const long nSize)
{   
    EVP_ENCODE_CTX ctxDecode; 
	int	nOut = 0;
    int nRet = 0;

    EVP_DecodeInit(&ctxDecode);
    nRet = EVP_DecodeUpdate(&ctxDecode, pOut, &nOut, pIn, nSize);
    if(nRet < 0)
    {
        //DEBUGPRINT(DEBUG_ERROR, "EVP_DecodeUpdate Error! @:%s @:%d\n", __FILE__, __LINE__);
        return nRet;
    }
        
    nRet = EVP_DecodeFinal(&ctxDecode, pOut, &nOut);
    if (nRet < 0)
    {
        //DEBUGPRINT(DEBUG_ERROR, "EVP_DecodeFinal Error! @:%s @:%d\n", __FILE__, __LINE__);
        return nRet;
    }

    return nRet;
}

/* MD5 解码 */
int md5check(/*unsigned */char strMD5Key[], const int nOutSize, const unsigned char *pIn, const int nInSize)
{
    MD5_CTX ctx;
    char strTmp[3] = {'\0'};
    int i = 0;
    int nSize = 0;

    nSize = nOutSize / 2;
    unsigned char strMD[nSize];

    MD5_Init(&ctx);
    MD5_Update(&ctx, pIn, nInSize);
    MD5_Final(strMD, &ctx);

    for(i=0; i<nSize; i++ )
    {
        sprintf(strTmp, "%02X", strMD[i]);
        strcat(strMD5Key, strTmp);
    }

    return 0;
}

