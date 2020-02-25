/**	
   @file NewsChannel.c
   @brief 应用程序主文件
   @note
   author: yangguozheng
   date: 2013-09-16
   mcdify record: creat this file.
   author: yangguozheng
   date: 2013-09-17
   modify record: add a function
 */
#include <string.h>
#include <stdlib.h>
#include <stdio.h>
#include <signal.h>
#include <arpa/inet.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <unistd.h>
#include <netinet/in.h>
#include <netinet/ip.h>
#include <netinet/ip_icmp.h>
#include <netdb.h>
#include <setjmp.h>
#include <errno.h>
#include <fcntl.h>
#include <pthread.h>

#define PACKET_SIZE     4096 //<包大小
//#define MAX_WAIT_TIME   3
//#define MAX_NO_PACKETS  1
#define exit(no)\
	return -1

#define PRIVATE static
//#define printf(args, ...)

PRIVATE char sendpacket[PACKET_SIZE]; //<发送缓冲区
PRIVATE char recvpacket[PACKET_SIZE]; //<接收缓冲区
PRIVATE int sockfd,datalen=56;        //<描述符，及数据长度
PRIVATE int nsend=0,nreceived=0;      //<已发包数及已接包数
PRIVATE struct sockaddr_in dest_addr; //<本地地址及目标地址
PRIVATE pid_t pid;                    //<进程id
PRIVATE struct sockaddr_in from;      //<回包地址
PRIVATE struct timeval tvrecv;        //<时间

PRIVATE int MAX_WAIT_TIME = 3;        //<最大等待时间
PRIVATE int MAX_NO_PACKETS = 3;       //<最大发送包数

typedef struct _tag_ping_info_
{
   char  acIpAddr[16];  //<待ping的ip地址
   int   nTimeOut;      //<超时时间，单位是秒
   int   nPackageNum;   //<发包数
}stPingInfo;

PRIVATE void statistics(int signo);                              //<信号处理句柄
PRIVATE unsigned short cal_chksum(unsigned short *addr,int len); //<校验和
PRIVATE int pack(int pack_no);                                   //<打包
PRIVATE void send_packet(void);                                  //<发送数据包
PRIVATE void recv_packet(void);                                  //<接收数据包
PRIVATE int unpack(char *buf,int len);                           //<解包
PRIVATE void tv_sub(struct timeval *out,struct timeval *in);     //<？

int ping_source_func(stPingInfo *pstPingInfo);                   //<接口

PRIVATE void statistics(int signo)
{       printf("\n--------------------PING statistics-------------------\n");
        printf("%d packets transmitted, %d received , %%%d lost\n",nsend,nreceived,
                        (nsend-nreceived)/nsend*100);
        close(sockfd);
//        exit(1);
//        return 0;
}
/*校验和算法*/
PRIVATE unsigned short cal_chksum(unsigned short *addr,int len)
{       int nleft=len;
        int sum=0;
        unsigned short *w=addr;
        unsigned short answer=0;
        
/*把ICMP报头二进制数据以2字节为单位累加起来*/
        while(nleft>1)
        {       sum+=*w++;
                nleft-=2;
        }
        /*若ICMP报头为奇数个字节，会剩下最后一字节。把最后一个字节视为一个2字节数据的高字节，这个2字节数据的低字节为0，继续累加*/
        if( nleft==1)
        {       *(unsigned char *)(&answer)=*(unsigned char *)w;
                sum+=answer;
        }
        sum=(sum>>16)+(sum&0xffff);
        sum+=(sum>>16);
        answer=~sum;
        return answer;
}
/*设置ICMP报头*/
PRIVATE int pack(int pack_no)
{       int i,packsize;
        struct icmp *icmp;
        struct timeval *tval;

        icmp=(struct icmp*)sendpacket;
        icmp->icmp_type=ICMP_ECHO;
        icmp->icmp_code=0;
        icmp->icmp_cksum=0;
        icmp->icmp_seq=pack_no;
        icmp->icmp_id=pid;
        packsize=8+datalen;
        tval= (struct timeval *)icmp->icmp_data;
        gettimeofday(tval,NULL);    /*记录发送时间*/
        icmp->icmp_cksum=cal_chksum( (unsigned short *)icmp,packsize); /*校验算法*/
        return packsize;
}

/*发送三个ICMP报文*/
PRIVATE void send_packet()
{       int packetsize;
        while( nsend<MAX_NO_PACKETS)
        {       nsend++;
                packetsize=pack(nsend); /*设置ICMP报头*/
                if( sendto(sockfd,sendpacket,packetsize,0,
                          (struct sockaddr *)&dest_addr,sizeof(dest_addr) )<0 )
                {       perror("sendto error");
                        continue;
                }
                usleep(500); /*每隔一秒发送一个ICMP报文*/
        }
}

/*接收所有ICMP报文*/
PRIVATE void recv_packet()
{       int n,fromlen;
        extern int errno;
#if 0
        signal(SIGALRM,statistics);
#endif
        fromlen=sizeof(from);
        fd_set fds;
		struct timeval timeout;

		FD_ZERO(&fds);
		int count = 0;
        while( nreceived<nsend)
        {
        	if(count > MAX_WAIT_TIME) {
				break;
			}
        	FD_SET(sockfd, &fds);
			timeout.tv_sec = 1;
			timeout.tv_usec = 0;
#if 0
			alarm(MAX_WAIT_TIME);
#endif
        	switch(select(sockfd+1,&fds,NULL,NULL,&timeout)) {
				case -1:return;
				case 0:++count;continue;
				default:
					if(FD_ISSET(sockfd, &fds)) {
						if( (n=recvfrom(sockfd,recvpacket,sizeof(recvpacket),0,
						                                (struct sockaddr *)&from,&fromlen)) <0)
						{       if(errno==EINTR)continue;
								perror("recvfrom error");
								continue;
						}
						gettimeofday(&tvrecv,NULL); /*记录接收时间*/
						if(unpack(recvpacket,n)==-1)continue;
						nreceived++;
					}
				break;
        	}
        }

}
/*剥去ICMP报头*/
PRIVATE int unpack(char *buf,int len)
{       int i,iphdrlen;
        struct ip *ip;
        struct icmp *icmp;
        struct timeval *tvsend;
        double rtt;

        ip=(struct ip *)buf;
        iphdrlen=ip->ip_hl<<2;    /*求ip报头长度,即ip报头的长度标志乘4*/
        icmp=(struct icmp *)(buf+iphdrlen); /*越过ip报头,指向ICMP报头*/
        len-=iphdrlen;            /*ICMP报头及ICMP数据报的总长度*/
        if( len<8)                /*小于ICMP报头长度则不合理*/
        {       printf("ICMP packets\'s length is less than 8\n");
                return -1;
        }
        /*确保所接收的是我所发的的ICMP的回应*/
        if( (icmp->icmp_type==ICMP_ECHOREPLY) && (icmp->icmp_id==pid) )
        {       tvsend=(struct timeval *)icmp->icmp_data;
                tv_sub(&tvrecv,tvsend); /*接收和发送的时间差*/
                rtt=tvrecv.tv_sec*1000+tvrecv.tv_usec/1000; /*以毫秒为单位计算rtt*/
                /*显示相关信息*/
                printf("%d byte from %s: icmp_seq=%u ttl=%d rtt=%.3f ms\n",
                        len,
                        inet_ntoa(from.sin_addr),
                        icmp->icmp_seq,
                        ip->ip_ttl,
                        rtt);
				return 0;
        }
        else    return -1;
		return 0;
}

PRIVATE int ping_process(int argc,char *argv[])
{       
		nsend=0;
		nreceived=0;
		struct hostent *host;
        struct protoent *protocol;
        unsigned long inaddr=0l;
        int waittime=MAX_WAIT_TIME;
        int size=50*1024;

        if(argc<2)
        {       printf("usage:%s hostname/IP address\n",argv[0]);
                exit(1);
        }

        if( (protocol=getprotobyname("icmp") )==NULL)
        {       perror("getprotobyname");
                exit(1);
        }

//        char *next = protocol->p_aliases;
//        while(NULL != next) {
//        	printf("==========%s==============", next);
//        	++next;
//        }

//        printf("==========%s %d==============", protocol->p_name, protocol->p_proto);
        /*生成使用ICMP的原始套接字,这种套接字只有root才能生成*/
        if( (sockfd=socket(AF_INET,SOCK_RAW,protocol->p_proto) )<0)
        {       perror("socket error");
                exit(1);
        }
        /* 回收root权限,设置当前用户权限*/
        setuid(getuid());
        /*扩大套接字接收缓冲区到50K这样做主要为了减小接收缓冲区溢出的
          的可能性,若无意中ping一个广播地址或多播地址,将会引来大量应答*/
        setsockopt(sockfd,SOL_SOCKET,SO_RCVBUF,&size,sizeof(size) );
        bzero(&dest_addr,sizeof(dest_addr));
        dest_addr.sin_family=AF_INET;
        /*判断是主机名还是ip地址*/
        if( inaddr=inet_addr(argv[1])==INADDR_NONE)
        {
        	if((host=gethostbyname(argv[1]) )==NULL) /*是主机名*/
                {       perror("gethostbyname error");
                        exit(1);
                }
//                memcpy( (char *)&dest_addr.sin_addr,host->h_addr,host->h_length);
        	memcpy( (char *)&dest_addr.sin_addr,host->h_addr,sizeof(struct in_addr));
        }
        else{    /*是ip地址*/
        	dest_addr.sin_addr.s_addr = inet_addr(argv[1]);
//                memcpy( (char *)&dest_addr,(char *)&inaddr,host->h_length);
        }
        /*获取main的进程id,用于设置ICMP的标志符*/
        pid=getpid();
        printf("PING %s(%s): %d bytes data in ICMP packets.\n",argv[1],
                        inet_ntoa(dest_addr.sin_addr),datalen);
        send_packet(); /*发送所有ICMP报文*/
        recv_packet(); /*接收所有ICMP报文*/
        statistics(SIGALRM); /*进行统计*/
//        if(nsend != nreceived) {
//        	printf("ping error\n");
//        	return -1;
//        }
        printf("ping ok\n");
        return nreceived;

}
/*两个timeval结构相减*/
PRIVATE void tv_sub(struct timeval *out,struct timeval *in)
{       if( (out->tv_usec-=in->tv_usec)<0)
        {       --out->tv_sec;
                out->tv_usec+=1000000;
        }
        out->tv_sec-=in->tv_sec;
}

#define P_FILE_NAME "/etc/protocols"
#define P_ITEM_ICMP "icmp    1       ICMP\n"
int ping_source_func(stPingInfo *pstPingInfo) {

	if(0 != access(P_FILE_NAME, O_RDWR)) {
		FILE *file = fopen(P_FILE_NAME, "w");
		if(NULL != file) {
			fwrite(P_ITEM_ICMP, strlen(P_ITEM_ICMP), 1, file);
			fclose(file);
		}
//		system("echo 'icmp    1       ICMP' > /etc/protocols");
	}

	char *argv[2];
	argv[1] = pstPingInfo->acIpAddr;
	if(0 != pstPingInfo->nTimeOut && 0 != pstPingInfo->nPackageNum) {
		MAX_WAIT_TIME = pstPingInfo->nTimeOut;
		MAX_NO_PACKETS = pstPingInfo->nPackageNum;
	}
	else {
		return -1;
	}
	return ping_process(2, argv);
}

#ifdef _PING_MAIN

int ithread_start(void *(run)(void *), void *arg) {
	pthread_t threadId;
	pthread_attr_t threadAttr;
	memset(&threadAttr,0,sizeof(pthread_attr_t));
	pthread_attr_init(&threadAttr);
	pthread_attr_setdetachstate(&threadAttr,PTHREAD_CREATE_DETACHED);
	int result = pthread_create(&threadId, &threadAttr, run, arg);
	pthread_attr_destroy(&threadAttr);
	printf("%d\n", threadId);
	return threadId;
}

void *p_run(void*args) {
	char **argv = (char **)args;
	stPingInfo pstPingInfo;
	memset(pstPingInfo.acIpAddr, 0, 16);
	memcpy(pstPingInfo.acIpAddr, argv[1], strlen(argv[1]));
	pstPingInfo.nPackageNum = 5;
	pstPingInfo.nTimeOut = 3;
	while(1) {
		ping_source_func(&pstPingInfo);
		usleep(500);
	}
}

int main(int argc, char *argv[]) {

	if(argc < 2) {
	  return -1;
	}
	
	ithread_start(p_run, argv);
	while(1) {
	  sleep(100);
	}
	return 0;
}
#endif
/*------------- The End -----------*/
