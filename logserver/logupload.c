/*
 * logupload.c
 *
 *  Created on: Jun 19, 2014
 *      Author: root
 */

#include <time.h>
#include <sys/select.h>
#include <sys/socket.h>
#include <stdlib.h>
#include <stdio.h>
#include <unistd.h>
#include <string.h>
#include <error.h>
#include <errno.h>
#include <pthread.h>
#include <signal.h>
#include <fcntl.h>
#include <sys/types.h>
#include <netinet/in.h>
#include <netdb.h>
#include <arpa/inet.h>
#include <netinet/tcp.h>
#include <stdarg.h>
#include <regex.h>
#include <openssl/ssl.h>

/**
   @brief assic字符转换为整形
   @author yangguozheng
   @param[out] to 目标
   @param[in] from 源字符
   @return 0
   @note
 */
inline int utils_atoi_a(char *to, const char *from) {
	if(*from > 'F' || *from < '0') {
		return -1;
	}
	if(*from > 'A' - 1) {
		*to = *from - 'A' + 10;
	}
	else if(*from > '0' - 1) {
		*to = *from - '0';
	}
//	DEBUGPRINT(DEBUG_INFO, "<>--[%d]--<>\n", *to);
	return 0;
}

/**
   @brief assic双字符转换为整形
   @author yangguozheng
   @param[out] to 目标
   @param[in] from 源双字符
   @return 0
   @note
 */
inline int utils_atoi_b(char *to, const char *from) {
	char b = 0;
	if(utils_atoi_a(&b, from) == -1) {
		return -1;
	}
	*to = 0;
	*to += b * 16;
	if(utils_atoi_a(&b, from + 1) == -1) {
		return -1;
	}
	*to += b;
	return 0;
}

/**
   @brief assic字符串转换为整形
   @author yangguozheng
   @param[out] to 目标
   @param[in] from 源字符串，必须为偶数个
   @return 0
   @note
 */
inline int utils_astois(char *to, const char *from, int size) {
	int i = 0;
	for(;i < size;++i) {
		if(utils_atoi_b(to, from) == -1) {
			return -1;
		}
//		DEBUGPRINT(DEBUG_INFO, "<>--[%d]--<>\n", *to);
		++to;
		from += 2;
	}
	return 0;
}

/**
   @brief 半字节整形转换为assic字符
   @author yangguozheng
   @param[out] to 目标
   @param[in] from 源整形
   @return 0
   @note
 */
inline int utils_itoa_a(char *to, const char from) {
	if(from > 0x09) {
		*to = 'A' + from - 0x0A;
	}
	else {
		*to = '0' + from - 0x00;
	}
	return 0;
}

/**
   @brief 一字节整形转换为assic双字符
   @author yangguozheng
   @param[out] to 目标
   @param[in] from 源整形
   @return 0
   @note
 */
inline int utils_itoa_b(char *to, const char from) {
	utils_itoa_a(to, (from&0xFF)>>4);
	utils_itoa_a(to + 1, from&0x0F);
	return 0;
}

/**
   @brief 整形转换为assic字符串
   @author yangguozheng
   @param[out] to 目标
   @param[in] from 源整形
   @return 0
   @note
 */
inline int utils_istoas(char *to, const char *from, int size) {
	int i = 0;
	for(;i < size;++i) {
		utils_itoa_b(to, *from);
		to += 2;
		++from;
	}
	return 0;
}

#define DEBUG    1
#define printf(format, arg...) fprintf(stdout, "debug %s: %d "format, __FUNCTION__, __LINE__, ##arg)
#define perror(format, arg...) fprintf(stdout, "error %s: %d %s "format, __FUNCTION__, __LINE__, strerror(errno), ##arg)

#if 1
int threadStart(void *(*run)(void *), void *arg) {
	pthread_t threadId;
	pthread_attr_t threadAttr;
	memset(&threadAttr,0,sizeof(pthread_attr_t));
	pthread_attr_init(&threadAttr);
	pthread_attr_setdetachstate(&threadAttr,PTHREAD_CREATE_DETACHED);
	int result = pthread_create(&threadId, &threadAttr, run, arg);
	pthread_attr_destroy(&threadAttr);
	printf("threadStart: threadId %ld\n", threadId);
	if(result != 0) {
		perror("threadStart: pthread_create");
		return -1;
	}
	return threadId;
}
#endif

//#define DEFAULT_PAGE  "/index.php"
#define DEFAULT_PAGE  "/iLog/api/upload_twoBase_log_info.ashx"
//#define DEFAULT_ADDR  "127.0.0.1"
#define DEFAULT_ADDR  "54.241.27.115"
//https://54.241.27.115:8443/iLog/api/
//#define DEFAULT_PORT  443
#define DEFAULT_PORT  8443

//54.241.27.115:8443/iLog/api/upload_twoBase_log_info.ashx

#define REQ_MAX_LEN   1024

#define LOG_BUF_FILE_NAME       	"/usr/share/logfile"
#define LOG_BUF_FILE_NAME_CMP      	"/usr/share/logfile.gz"
#define LOG_BUF_FILE_NAME_CMD       "gzip -c /usr/share/logfile > /usr/share/logfile.gz"

//#include <openssl/bio.h>

typedef struct Self_t {
	char request[REQ_MAX_LEN];
	char rBody[REQ_MAX_LEN];
	char response[REQ_MAX_LEN];
	char *addr;
	int port;
	char *page;
	int sockfd;
	char mac[25];
	char version[128];
	char hversion[128];
	char cameraModel[8];
	char clientType[8];
}Self_t;

Self_t self_t = {{0}, {0}, -1, {0}, {0}, {0}, {0}};

int log_request(char *request, int reqlen, char *body, int blen, char *response, int reslen, int sock, int timeout, int opt) {
	if(NULL == request || reqlen <= 0 || sock <= 0) {
		return -1;
	}
	int ret;
	/* SSL初始化 */
	SSL *ssl;
	SSL_CTX *ctx;
	regex_t re;
	regex_t headEndRe;
	regcomp(&headEndRe, "\r\n\r\n", REG_EXTENDED);
	regcomp(&re, "Content-Length:[ ]*([0-9]{1,})\r\n", REG_EXTENDED);
	SSL_library_init();
	OpenSSL_add_all_algorithms();
	SSL_load_error_strings();
	ctx = SSL_CTX_new(SSLv23_client_method());
	if (ctx == NULL) {
		perror("SSL_CTX_new");
		return(-1);
	}
	printf("SSL_CTX_new\n");

	ssl = SSL_new(ctx);
	if (ssl == NULL) {
		perror("SSL_new");
		SSL_CTX_free(ctx);
		return(-1);
	}
	printf("SSL_new\n");

	/* 把socket和SSL关联 */
	ret = SSL_set_fd(ssl, sock);
	if (ret == 0) {
		perror("SSL_set_fd");
		SSL_free(ssl);
		SSL_CTX_free(ctx);
		return(-1);
	}
	printf("SSL_set_fd\n");
	ret = SSL_connect(ssl);
	if (ret != 1) {
		perror("log_httpsRequest: SSL_connect");
		SSL_shutdown(ssl);
		SSL_free(ssl);
		SSL_CTX_free(ctx);
		return(-1);
	}

	printf("SSL_connect\n");

	int send = 0;
	int totalsend = 0;
	int nbytes = strlen(request);
	printf("send head\n");
	//send head
	while (totalsend < nbytes) {
		send = SSL_write(ssl, request + totalsend, nbytes - totalsend);
		if (send == -1) {
//			if (DEBUG)
//				ERR_print_errors_fp(stderr);
			ret = -1;
			goto rDone;
		}
		totalsend += send;
		if (DEBUG)
			printf("%d bytes send OK!\n", totalsend);
	}
//	printf("http request head %s\n", request);
	printf("send body\n");
	//send body
	if(body != NULL && blen > 0) {
		send = 0;
		totalsend = 0;
		nbytes = blen;
		while (totalsend < nbytes) {
			send = SSL_write(ssl, body + totalsend, nbytes - totalsend);
			if (send == -1) {
				perror("SSL_write");
				ret = -1;
				goto rDone;
			}
			totalsend += send;
			if (DEBUG)
				printf("%d bytes send OK!\n", totalsend);
		}
	}
	printf("send body over\n");
	regmatch_t match[2];
	int rLen = 0;
	send = 0;
	totalsend = 0;
//	nbytes = reqlen;
//	memset(request, '\0', reqlen);
	if(NULL == response || reslen <= 0) {
		goto rDone;
	}
	memset(response, '\0', reslen);
	printf("get response\n");
	while(1) {
		if(0 == regexec(&headEndRe, response, 1, match, REG_EXTENDED)) {
			break;
		}
		if(rLen == reslen) {
			break;
		}
		ret = SSL_read(ssl, response + rLen, reslen - rLen);
		if(ret > 0) {
			rLen += ret;
		} else if(-1 == ret) {
			ret = -1;
			goto rDone;
		}
	}
	int rLenBody = 0;
	int rLenHead = match[0].rm_eo;
	if(0 == regexec(&re, response, 2, match, REG_EXTENDED)) {
//		printf("%c %c\n", *(char *)(buf + match->rm_so), *(char *)(buf + match[1].rm_so));
		sscanf(response + match[1].rm_so, "%d", &rLenBody);
		printf("%d\n", rLenBody);
		if(rLenBody <= 0 || rLenBody + rLenHead >= reslen) {
			return rLen;
		}
		while(1) {
			if(rLen == rLenBody + rLenHead) {
				break;
			}
			ret = SSL_read(ssl, response + rLen, rLenBody + rLenHead - rLen);
			if(ret > 0) {
				rLen += ret;
			} else if(-1 == ret) {
				ret = -1;
				goto rDone;
			}
		}
//		printf("recvHttpSockData\n");
	}
	rDone: {
		SSL_shutdown(ssl);
		SSL_free(ssl);
		SSL_CTX_free(ctx);
		printf("rDone %d\n", ret);
	}
	regfree(&headEndRe);
	printf("rDone rDone rDone regfree(&headEndRe) %d\n", ret);
	regfree(&re);
//	printf("http response head %s\n", request);
	printf("rDone rDone rDone rDone %d\n", ret);
	return ret;
}

int getIpByHost(char *ip, const char *host)
{

    int nRet = 0;
    char **ppList;
    struct hostent *pHostent;
    char strIP[40+1];

    if ((NULL==ip) || (NULL==host))
    {
        perror("NULL");
        return -1;
    }

    if((pHostent = gethostbyname(host)) == NULL)
    {
    	perror("NULL");
        return -1;
    }

    switch(pHostent->h_addrtype)
    {
       case AF_INET:
       case AF_INET6:
            ppList = pHostent->h_addr_list;
            for (; *ppList!=NULL; ppList++)
            {
                sprintf(ip, "%s", inet_ntop(pHostent->h_addrtype, *ppList, strIP, sizeof(strIP)));
                if (strlen(ip) > 0)
                {
                    break;
                }
            }
            break;
       default:
    	    perror("default NULL");
            nRet = -3;
            break;
    }

    if (strlen(ip)>0)
    {
        if (ip[strlen(ip)-1] == '\n')
        {
            ip[strlen(ip)-1] = '\0';
        }
    }

    return nRet;
}

/**
 @brief 连接套接字
 @author yangguozheng
 @param[sockfd] 套接字描述
 @param[saptr] 地址信息
 @param[salen] 地址信息长度
 @param[nsec] 超时时间
 @return 0
 @note
 */
int conn_nonb(int sockfd, const struct sockaddr_in *saptr, socklen_t salen, int nsec)
{
    int flags, n, error, code;
    socklen_t len;
    fd_set wset;
    struct timeval tval;

    flags = fcntl(sockfd, F_GETFL, 0);
    fcntl(sockfd, F_SETFL, flags | O_NONBLOCK);

    error = 0;
    if ((n == connect(sockfd, saptr, salen)) == 0) {
        goto conn_nonb_done;
    } else if (n < 0 && errno != EINPROGRESS){
        return (-1);
    }

    /* Do whatever we want while the connect is taking place */

    FD_ZERO(&wset);
    FD_SET(sockfd, &wset);
    tval.tv_sec = nsec;
    tval.tv_usec = 0;

    if ((n = select(sockfd+1, NULL, &wset,
                    NULL, nsec ? &tval : NULL)) == 0) {
        close(sockfd);  /* timeout */
        errno = ETIMEDOUT;
        return (-1);
    }

    if (FD_ISSET(sockfd, &wset)) {
        len = sizeof(error);
        code = getsockopt(sockfd, SOL_SOCKET, SO_ERROR, &error, &len);
        /* 如果发生错误，Solaris实现的getsockopt返回-1，
         * 把pending error设置给errno. Berkeley实现的
         * getsockopt返回0, pending error返回给error.
         * 我们需要处理这两种情况 */
        if (code < 0 || error) {
            close(sockfd);
            if (error)
                errno = error;
            return (-1);
        }
    } else {
        fprintf(stderr, "select error: sockfd not set");
        return -1;
    }

conn_nonb_done:
    fcntl(sockfd, F_SETFL, flags);  /* restore file status flags */
    return (0);
}

#include <sys/stat.h>

unsigned long get_file_size(const char *path)
{
	unsigned long filesize = -1;
	struct stat statbuff;
	if(stat(path, &statbuff) < 0){
		return filesize;
	}else{
		filesize = statbuff.st_size;
	}
	return filesize;
}

int https_client_main(int argc, char *argv[]) {
	Self_t *self = &self_t;

	printf("%s\n", LOG_BUF_FILE_NAME_CMD);
	system(LOG_BUF_FILE_NAME_CMD);

	int logSize = get_file_size(LOG_BUF_FILE_NAME_CMP);
	if(logSize <= 0 || logSize > 1024 * 1024 * 2) {
		perror("%s\n", LOG_BUF_FILE_NAME_CMP);
		return 0;
	}

	void *sBody = malloc(logSize);

	FILE *file = fopen(LOG_BUF_FILE_NAME_CMP, "r");
	if(NULL == file) {
		perror("fopen \n");
		return 0;
	}
	fread(sBody, logSize, 1, file);
	fclose(file);

	int ret = 0;
	char ip[20];
	if(argc == 1) {
		self->addr = DEFAULT_ADDR;
		self->port = DEFAULT_PORT;
		self->page = DEFAULT_PAGE;
	} else if(argc == 2) {
		self->addr = argv[1];
		self->port = DEFAULT_PORT;
		self->page = DEFAULT_PAGE;
	} else if(argc == 3) {
		self->addr = argv[1];
		self->port = atoi(argv[2]);
		self->page = DEFAULT_PAGE;
	} else if(argc == 4) {
		self->addr = argv[1];
		self->port = atoi(argv[2]);
		self->page = argv[3];
	}

	int sockfd = socket(AF_INET, SOCK_STREAM, 0);
	if(sockfd == -1) {
		perror("socket");
		return -1;
	}
	printf("socket ok\n");
	struct sockaddr_in sAddr;
	if(0 != getIpByHost(ip, self->addr)) {
		perror("getIpByHost");
		return -1;
	}
	printf("%s\n", ip);
	bzero(&sAddr,sizeof(struct sockaddr_in));
	sAddr.sin_family = AF_INET;
	sAddr.sin_addr.s_addr = inet_addr(ip);
	sAddr.sin_port = htons(self->port);
	int sockOpt = 1; /*turn on*/

	if(setsockopt(sockfd, SOL_SOCKET, SO_REUSEADDR, &sockOpt, sizeof(int)) != 0) {
		perror("setsockopt");
		return -1;
	}
	printf("setsockopt ok\n");
	if(0 != conn_nonb(sockfd, &sAddr, sizeof(struct sockaddr_in), 3)) {
		perror("conn_nonb\n");
		return -1;
	}

	struct timeval timeOut = {0, 5000000};
	setsockopt(sockfd,SOL_SOCKET,SO_SNDTIMEO,&timeOut,sizeof(struct timeval));
	setsockopt(sockfd,SOL_SOCKET,SO_RCVTIMEO,&timeOut,sizeof(struct timeval));

//	sprintf(self->rBody, "headinfo={\"camid\":\"B8A94E000006\",\"camenr\":\"hUmUJ88W910FMe5i\",\"cmpt\":\"M3S\",\"chwv\":\"1.2.3.212V\",\"hwv\":\"0.0.4.0\",\"sc\":\"4d5ba53a2ed546c7b0b03bf13e66f079\",\"sv\":\"7f08031a888e40e2957607dc87cc9d10\"}"
//			"&content={\"UUID\":\"0808080808080808\",\"ClientType\":\"iBaby\",\"ClientHardwareModels\":\"M3S\",\"ClientVersionNumber\":\"1.2.3.273\",\"ClientSystemVersionNumber\":\"0.0.4.0\",FileInfo\":\"0\"}");

	sprintf(self->rBody, "Head: {\"UUID\":\"0808080808080808\",\"ClientType\":\"iBaby\",\"ClientHardwareModels\":\"M3S\",\"ClientVersionNumber\":\"1.2.3.273\",\"ClientSystemVersionNumber\":\"0.0.4.0\",FileInfo\":\"0\","
			"\"sc\":\"b76aea00b2734b65884b1364912a3828\",\"sv\":\"f46e123be95942598700f8ee1b720006\",\"Extend\":\".txt\"}");


	sprintf(self->request, "POST %s HTTP/1.1\r\n"
			"Host: %s\r\n"
			"Head: {\"UUID\":\"%s\",\"ClientType\":\"%s\",\"ClientHardwareModels\":\"%s\",\"ClientVersionNumber\":\"%s\",\"ClientSystemVersionNumber\":\"%s\","
			"\"sc\":\"b76aea00b2734b65884b1364912a3828\",\"sv\":\"f46e123be95942598700f8ee1b720006\",\"Extend\":\".gz\"}\r\n"
			"Content-Type: application/octet-stream\r\n"
//			"Content-Type: application/x-www-form-urlencoded\r\n"
			"Content-Length: %d\r\n\r\n",
			self->page, self->addr, self->mac + 12, self->clientType, self->cameraModel, self->version, self->hversion,
//			strlen(self->rBody));
			logSize);
	printf("request %s%s\n", self->request, self->rBody);

	ret = log_request(self->request, REQ_MAX_LEN, sBody, logSize, self->response, REQ_MAX_LEN, sockfd, 0, 0);
//	ret = log_request(self->request, REQ_MAX_LEN, self->rBody, strlen(self->rBody), self->response, REQ_MAX_LEN, sockfd, 0, 0);
	free(sBody);
	shutdown(sockfd, SHUT_RDWR);
	close(sockfd);
	printf("response %s\n", self->response);
	return 0;
}

void *testRun(void *arg) {
	FILE *file = fopen("a", "r");
	char buf[2];
	while(1) {
		fseek(file, SEEK_SET, 0);
		fread(buf, 1, 1, file);
		printf("%d\n", buf[0]);
//		fflush(file);
		usleep(100000);
	}
	fclose(file);
	return 0;
}

#include <sys/shm.h>
#include <sys/stat.h>

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
	int lRet = -1;
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

int main(int argc, char *argv[]) {
//	threadStart(testRun, NULL);
	Self_t *self = &self_t;
	int upIndex = 0;
	key_t key;
	key = ftok(".", 10086);
	int segment_id = shmget(key, getpagesize(), IPC_CREAT);

	FILE *file = fopen("/usr/share/info.txt", "r");
	char buf[1024] = {0};
	if(NULL != file) {
		fscanf(file, "%s", buf);
		strcpy(self->version, buf + strlen("firmware="));
		printf("%s\n", self->version);
		fscanf(file, "%s", buf);
		strcpy(self->clientType, buf + strlen("camera_type="));
		printf("%s\n", self->clientType);
		fscanf(file, "%s", buf);
		strcpy(self->hversion, buf + strlen("hardware="));
		printf("%s\n", self->hversion);
		fscanf(file, "%s", buf);
		strcpy(self->cameraModel, buf + strlen("camera_model="));
		printf("%s\n", self->cameraModel);
		fclose(file);
	}

	read_mac_addr_from_flash(self->mac);
//	self->mac[13] = '\0';

//	return 0;

	char *shareData = (char*)shmat(segment_id, 0, 0);
	while(1) {
		if(upIndex < *(int *)shareData) {
			upIndex = *(int *)shareData;
			https_client_main(argc, argv);
			printf("upIndex %d\n", *(int *)shareData);
		} else {
			printf("upIndex %d\n", *(int *)shareData);
		}
		sleep(10);
	}
	return 0;
}






