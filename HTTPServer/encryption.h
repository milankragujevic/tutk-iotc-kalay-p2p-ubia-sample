#ifndef _encryption_h
#define _encryption_h


/* RSA 公钥加密 */
extern int RSAPublicEncryption_IPU(char *STRKEYN,char *STRKEYE,char *pCipherText, const char *pClearText, const int nPandding);
extern int RSAPublicEncryption(unsigned char *ciphertext, const unsigned char *cleartext, const int nPandding);

/* RSA 私钥解密 */
extern int RSAPrivateDecryption(unsigned char *ciphertext, const unsigned char *cleartext, const int nPandding);


/* BASE64 编码 */
extern void Base64Encode(unsigned char *pOut, const unsigned char *pIn, const long nSize);

/* BASE64 解码 */
extern int Base64Decode(unsigned char *pOut, const unsigned char *pIn, const long nSize);

/* MD5 校验 */
extern int md5check(unsigned char strMD5Key[], const int nOutSize, const unsigned char *pIn, const int nInSize);

/* Base64解码 */
extern unsigned char* DeBase64Code(const char* source, const int sourceLength, int *nLeng);

#endif


