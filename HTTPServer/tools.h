/*
 * tools.h
 *
 *  Created on: Apr 9, 2013
 *      Author: root
 */

#ifndef TOOLS_H_
#define TOOLS_H_
extern int ReadFile2Binary(char **pFileBuffer, const char *pFileName);
extern int ChangeBase64ToURL(char *pCamEnrIDToURLEncode, const char *pCamEncryptionID);
extern int URLDecode(char *str, int len);
extern char *URLEncode(char const *s, int len, int *new_length);
extern int ClearCRLF(char *pStrText);
extern int CreatIPUEncryptionID(char *pCamEncryptionID, const char *pRandom,char * strkeyN,char *strkeyE);
extern int DecodeEncryptionID(char *EncryptionID);
extern int ReadFile(char **pFileBuffer, const char *pFileName);
extern int SetStableFlag(void);
#endif /* TOOLS_H_ */
