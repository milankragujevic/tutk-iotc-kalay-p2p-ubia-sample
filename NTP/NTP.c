/*
 * ntpworker.c
 *
 *  Created on: Aug 26, 2013
 *      Author: wangjiannan
 */



#include "NTP/NTP.h"

#include "common/thread.h"
#include "common/common.h"
#include "common/msg_queue.h"
#include "common/logfile.h"

//#include "ntpworker.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <netdb.h>     /* gethostbyname */
#include <arpa/inet.h>
#include <time.h>
#include <unistd.h>
#include <errno.h>
#include <time.h>
#include <sys/time.h>

#include <stdint.h>

typedef uint32_t u32;  /* universal for C99 */

struct ntp_control {
	u32 time_of_send[2];
	int live;
	int set_clock;   /* non-zero presumably needs root privs */
	int probe_count;
	int cycle_time;
	int goodness;
	int cross_check;
	char serv_addr[4];
};

/* Default to the RFC-4330 specified value */
#ifndef MIN_INTERVAL
#define MIN_INTERVAL 15
#endif

#define NTP_PORT (123)

struct ntptime {
	unsigned int coarse;
	unsigned int fine;
};

#define JAN_1970        0x83aa7e80      /* 2208988800 1970 - 1900 in seconds */

/* How to multiply by 4294.967296 quickly (and not quite exactly)
 * without using floating point or greater than 32-bit integers.
 * If you want to fix the last 12 microseconds of error, add in
 * (2911*(x))>>28)
 */
#define NTPFRAC(x) ( 4294*(x) + ( (1981*(x))>>11 ) )

/* Converts NTP delay and dispersion, apparently in seconds scaled
 * by 65536, to microseconds.  RFC-1305 states this time is in seconds,
 * doesn't mention the scaling.
 * Should somehow be the same as 1000000 * x / 65536
 */
#define sec2u(x) ( (x) * 15.2587890625 )

/* The reverse of the above, needed if we want to set our microsecond
 * clock (via clock_settime) based on the incoming time in NTP format.
 * Basically exact.
 */
#define USEC(x) ( ( (x) >> 12 ) - 759 * ( ( ( (x) >> 10 ) + 32768 ) >> 16 ) )

int NTP_set_time();
int get_after_NTP_time(long int T0);
void threadSynchronization_NTP();
void  * NTP_Thread(void *arg);

int time_flag = 1;
int t_time_flag = 1;

void setup_receive(int usd, unsigned int interface, short port);
void setup_transmit(int usd, char *host, short port, struct ntp_control *ntpc);
void primary_loop( struct ntp_control *ntpc,netset_state_t *pstNetSetState);
void stuff_net_addr(struct in_addr *p, char *hostname);
void send_packet(int usd, u32 time_sent[2]);
void ntpc_gettime(u32 *time_coarse, u32 *time_fine);
void get_packet_timestamp(int usd, struct ntptime *udp_arrival_ntp);
int check_source(int data_len, struct sockaddr *sa_source, unsigned int sa_len, struct ntp_control *ntpc);
int rfc1305print(u32 *data, struct ntptime *arrival, struct ntp_control *ntpc, int *error);
double ntpdiff( struct ntptime *start, struct ntptime *stop);
int get_current_freq(void);
void set_time(struct ntptime *new);
int contemplate_data(unsigned int absolute, double skew, double errorbar, int freq);
int set_freq(int new_freq);

int Create_NTP_Thread(void) {
	int Create_NTP;
	Create_NTP = thread_start(NTP_Thread);
	return Create_NTP;
}

void  * NTP_Thread(void *arg){
//start Init NTP

//	int usd;  /* socket */
//
//	short int udp_local_port=0;   /* default of 0 means kernel chooses */
//	char *hostname="203.117.180.36";          /* must be set */

	struct ntp_control ntpc;
	ntpc.live=0;
	ntpc.set_clock=1;
	ntpc.probe_count=0;           /* default of 0 means loop forever */
	ntpc.cycle_time=600;          /* seconds */
	ntpc.goodness=0;
	ntpc.cross_check=1;

	if (ntpc.set_clock && !ntpc.live && !ntpc.goodness && !ntpc.probe_count) {
		ntpc.probe_count = 1;
	}

	/* respect only applicable MUST of RFC-4330 */
	if (ntpc.probe_count != 1 && ntpc.cycle_time < MIN_INTERVAL) {
		ntpc.cycle_time = MIN_INTERVAL;
	}

	netset_state_t *pstNetSetState = malloc(sizeof(netset_state_t));
	memset(pstNetSetState,0,sizeof(netset_state_t));

//end of Init_NTP
	threadSynchronization_NTP();

//	while(1){
//		get_cur_net_state(pstNetSetState);
//		if(0 == pstNetSetState->lNetConnectState){
//			/* Startup sequence */
//
//
//
//			break;
//		}
//		usleep(300000);
//	}
//	if ((usd=socket(AF_INET,SOCK_DGRAM,IPPROTO_UDP))==-1)
//					{perror ("socket");}
//
//	setup_receive(usd, INADDR_ANY, udp_local_port);
//
//	setup_transmit(usd, hostname, NTP_PORT, &ntpc);

	primary_loop(&ntpc,pstNetSetState);

	return NULL;
}

void threadSynchronization_NTP() {

	rNTP = 1;
	while(rChildProcess == 0) {
		thread_usleep(0);
	}
	thread_usleep(0);
}

void setup_receive(int usd, unsigned int interface, short port)
{
	struct sockaddr_in sa_rcvr;
	memset(&sa_rcvr,0,sizeof sa_rcvr);
	sa_rcvr.sin_family=AF_INET;
	sa_rcvr.sin_addr.s_addr=htonl(interface);
	sa_rcvr.sin_port=htons(port);
	if(bind(usd,(struct sockaddr *) &sa_rcvr,sizeof sa_rcvr) == -1) {
		perror("bind");
		fprintf(stderr,"could not bind to udp port %d\n",port);
		exit(1);
	}
	/* listen(usd,3); this isn't TCP; thanks Alexander! */
}

void setup_transmit(int usd, char *host, short port, struct ntp_control *ntpc)
{
	struct sockaddr_in sa_dest;
	memset(&sa_dest,0,sizeof sa_dest);
	sa_dest.sin_family=AF_INET;
	stuff_net_addr(&(sa_dest.sin_addr),host);
	memcpy(ntpc->serv_addr,&(sa_dest.sin_addr),4); /* XXX asumes IPv4 */
	sa_dest.sin_port=htons(port);
	if (connect(usd,(struct sockaddr *)&sa_dest,sizeof sa_dest)==-1){
		perror("connect");exit(1);
	}else{
		printf("connect is OK\n");
	}
}

void stuff_net_addr(struct in_addr *p, char *hostname)
{
	struct hostent *ntpserver;
	ntpserver=gethostbyname(hostname);
	if (ntpserver == NULL) {
		herror(hostname);
	}else{
		if (ntpserver->h_length != 4) {
			/* IPv4 only, until I get a chance to test IPv6 */
			fprintf(stderr,"oops %d\n",ntpserver->h_length);
		}
		memcpy(&(p->s_addr),ntpserver->h_addr_list[0],4);
	}
}

void primary_loop( struct ntp_control *ntpc,netset_state_t *pstNetSetState){

	int usd;  /* socket */

	short int udp_local_port=0;   /* default of 0 means kernel chooses */
	char *hostname="203.117.180.36";          /* must be set */

	time_t t00;
	fd_set fds;
	struct sockaddr sa_xmit;
	int i ,pack_len,probes_sent,error,ressnd;
	socklen_t sa_xmit_len;
	struct timeval to;
	struct ntptime udp_arrival_ntp;
	static u32 incoming_word[325];
#define incoming ((char *) incoming_word)
#define sizeof_incoming (sizeof incoming_word)

	//int network_flag = 0;

	if (debug) printf("Listening...\n");

	probes_sent=0;
	sa_xmit_len=sizeof sa_xmit;
	ressnd = 1;
	long int oldTime = 0;
	while(1){

		get_cur_net_state(pstNetSetState);
		//printf("__LINE__:%d\n",__LINE__);
    	if(pstNetSetState->lNetConnectState == NET_CONNECT_SUCCESS){

    		if(time_flag != 0){

    			DEBUGPRINT(DEBUG_INFO,"--------------NTP begin Guozhiin ---------------------\n");

    			if ((usd=socket(AF_INET,SOCK_DGRAM,IPPROTO_UDP))==-1)
    							{perror ("socket");}

    			setup_receive(usd, INADDR_ANY, udp_local_port);

    			setup_transmit(usd, hostname, NTP_PORT, &ntpc);

    			send_packet(usd,ntpc->time_of_send);

				to.tv_sec=3;
				to.tv_usec=0;

				FD_ZERO(&fds);
				FD_SET(usd,&fds);
				i=select(usd+1,&fds,NULL,NULL,&to);  /* Wait on read or error */
				switch (i){
					case -1:
						printf("__LINE__:%d\n",__LINE__);
						//if (errno != EINTR) perror("select");
						ressnd = 1;
						break;
					case 0:
						ressnd = 1;
						break;
					default:
						if(FD_ISSET(usd,&fds)){
							pack_len=recvfrom(usd,incoming,sizeof_incoming,0,&sa_xmit,&sa_xmit_len);
							error = ntpc->goodness;
							if (pack_len<0) {
								//ressnd = 1;
								perror("recvfrom");
							}else if(pack_len>0 && (unsigned)pack_len<sizeof_incoming){

								DEBUGPRINT(DEBUG_INFO,"--------------NTP recv OK Guozhiin ---------------------\n");

								get_packet_timestamp(usd, &udp_arrival_ntp);
								if (check_source(pack_len, &sa_xmit, sa_xmit_len, ntpc)!=0) {
									DEBUGPRINT(DEBUG_INFO,"--------------check_source ---------------------\n");
									continue;
								}
								if (rfc1305print(incoming_word, &udp_arrival_ntp, ntpc, &error)!=0) {
									DEBUGPRINT(DEBUG_INFO,"--------------rfc1305print ---------------------\n");
									continue;
								}
								ressnd = 0;

								DEBUGPRINT(DEBUG_INFO,"--------------NTP SUM OK Guozhiin ---------------------\n");
								/* udp_handle(usd,incoming,pack_len,&sa_xmit,sa_xmit_len); */
							}else{
								ressnd = 0;
								printf("Ooops.  pack_len=%d\n",pack_len);
								fflush(stdout);
							}
						}
					    break;
				}
				close(usd);

    			if(0 == ressnd){

    				DEBUGPRINT(DEBUG_INFO,"--------------NTP Back OK Guozhiin ---------------------\n");

    				oldTime = time(&t00);

    				t_time_flag = 0;
    				time_flag = 0;
    			}else{

    			}
    		}else{
    			get_after_NTP_time(oldTime);
    		}
     	}else{
     		//xxxxxxxxxxxxxxxxxxxxxxx
     	}
		usleep(1000000);
	}

}

void send_packet(int usd, u32 time_sent[2])
{
	u32 data[12];
#define LI 0
#define VN 3
#define MODE 3
#define STRATUM 0
#define POLL 4
#define PREC -6

	if (debug) fprintf(stderr,"Sending ...\n");
	if (sizeof data != 48) {
		fprintf(stderr,"size error\n");
		return;
	}
	memset(data,0,sizeof data);
	data[0] = htonl (
		( LI << 30 ) | ( VN << 27 ) | ( MODE << 24 ) |
		( STRATUM << 16) | ( POLL << 8 ) | ( PREC & 0xff ) );
	data[1] = htonl(1<<16);  /* Root Delay (seconds) */
	data[2] = htonl(1<<16);  /* Root Dispersion (seconds) */
	ntpc_gettime(time_sent, time_sent+1);
	data[10] = htonl(time_sent[0]); /* Transmit Timestamp coarse */
	data[11] = htonl(time_sent[1]); /* Transmit Timestamp fine   */
	send(usd,data,48,0);
}

void get_packet_timestamp(int usd, struct ntptime *udp_arrival_ntp)
{
	(void) usd;  /* not used */
	ntpc_gettime(&udp_arrival_ntp->coarse, &udp_arrival_ntp->fine);
}

void ntpc_gettime(u32 *time_coarse, u32 *time_fine)
{
	/* Traditional Linux way to get the system time
	 */
	struct timeval now;
	gettimeofday(&now, NULL);
	*time_coarse = now.tv_sec + JAN_1970;
	*time_fine   = NTPFRAC(now.tv_usec);
}

int check_source(int data_len, struct sockaddr *sa_source, unsigned int sa_len, struct ntp_control *ntpc)
{
	struct sockaddr_in *sa_in=(struct sockaddr_in *)sa_source;
	(void) sa_len;  /* not used */
	if (debug) {
		printf("packet of length %d received\n",data_len);
		if (sa_source->sa_family==AF_INET) {
			printf("Source: INET Port %d host %s\n",
				ntohs(sa_in->sin_port),inet_ntoa(sa_in->sin_addr));
		} else {
			printf("Source: Address family %d\n",sa_source->sa_family);
		}
	}
	/* we could check that the source is the server we expect, but
	 * Denys Vlasenko recommends against it: multihomed hosts get it
	 * wrong too often. */
#if 0
	if (memcmp(ntpc->serv_addr, &(sa_in->sin_addr), 4)!=0) {
		return 1;  /* fault */
	}
#else
	(void) ntpc; /* not used */
#endif
	if (NTP_PORT != ntohs(sa_in->sin_port)) {
		return 1;  /* fault */
	}
	return 0;
}

/* Does more than print, so this name is bogus.
 * It also makes time adjustments, both sudden (-s)
 * and phase-locking (-l).
 * sets *error to the number of microseconds uncertainty in answer
 * returns 0 normally, 1 if the message fails sanity checks
 */
int rfc1305print(u32 *data, struct ntptime *arrival, struct ntp_control *ntpc, int *error)
{
/* straight out of RFC-1305 Appendix A */
	int li, vn, mode, stratum, poll, prec;
	int delay, disp, refid;
	struct ntptime reftime, orgtime, rectime, xmttime;
	double el_time,st_time,skew1,skew2;
	int freq;
#ifdef ENABLE_DEBUG
	const char *drop_reason=NULL;
#endif

#define Data(i) ntohl(((u32 *)data)[i])
	li      = Data(0) >> 30 & 0x03;
	vn      = Data(0) >> 27 & 0x07;
	mode    = Data(0) >> 24 & 0x07;
	stratum = Data(0) >> 16 & 0xff;
	poll    = Data(0) >>  8 & 0xff;
	prec    = Data(0)       & 0xff;
	if (prec & 0x80) prec|=0xffffff00;
	delay   = Data(1);
	disp    = Data(2);
	refid   = Data(3);
	reftime.coarse = Data(4);
	reftime.fine   = Data(5);
	orgtime.coarse = Data(6);
	orgtime.fine   = Data(7);
	rectime.coarse = Data(8);
	rectime.fine   = Data(9);
	xmttime.coarse = Data(10);
	xmttime.fine   = Data(11);
#undef Data

	if (debug) {
	printf("LI=%d  VN=%d  Mode=%d  Stratum=%d  Poll=%d  Precision=%d\n",
		li, vn, mode, stratum, poll, prec);
	printf("Delay=%.1f  Dispersion=%.1f  Refid=%u.%u.%u.%u\n",
		sec2u(delay),sec2u(disp),
		refid>>24&0xff, refid>>16&0xff, refid>>8&0xff, refid&0xff);
	printf("Reference %u.%.6u\n", reftime.coarse, USEC(reftime.fine));
	printf("(sent)    %u.%.6u\n", ntpc->time_of_send[0], USEC(ntpc->time_of_send[1]));
	printf("Originate %u.%.6u\n", orgtime.coarse, USEC(orgtime.fine));
	printf("Receive   %u.%.6u\n", rectime.coarse, USEC(rectime.fine));
	printf("Transmit  %u.%.6u\n", xmttime.coarse, USEC(xmttime.fine));
	printf("Our recv  %u.%.6u\n", arrival->coarse, USEC(arrival->fine));
	}
	el_time=ntpdiff(&orgtime,arrival);   /* elapsed */
	st_time=ntpdiff(&rectime,&xmttime);  /* stall */
	skew1=ntpdiff(&orgtime,&rectime);
	skew2=ntpdiff(&xmttime,arrival);
	freq=get_current_freq();
	if (debug) {
	printf("Total elapsed: %9.2f\n"
	       "Server stall:  %9.2f\n"
	       "Slop:          %9.2f\n",
		el_time, st_time, el_time-st_time);
	printf("Skew:          %9.2f\n"
	       "Frequency:     %9d\n"
	       " day   second     elapsed    stall     skew  dispersion  freq\n",
		(skew1-skew2)/2, freq);
	}

	/* error checking, see RFC-4330 section 5 */
#ifdef ENABLE_DEBUG
#define FAIL(x) do { drop_reason=(x); goto fail;} while (0)
#else
#define FAIL(x) goto fail;
#endif
	if (ntpc->cross_check) {
		if (li == 3) FAIL("LI==3");  /* unsynchronized */
		if (vn < 3) FAIL("VN<3");   /* RFC-4330 documents SNTP v4, but we interoperate with NTP v3 */
		if (mode != 4) FAIL("MODE!=3");
		if (orgtime.coarse != ntpc->time_of_send[0] ||
		    orgtime.fine   != ntpc->time_of_send[1] ) FAIL("ORG!=sent");
		if (xmttime.coarse == 0 && xmttime.fine == 0) FAIL("XMT==0");
		if (delay > 65536 || delay < -65536) FAIL("abs(DELAY)>65536");
		if (disp  > 65536 || disp  < -65536) FAIL("abs(DISP)>65536");
		if (stratum == 0) FAIL("STRATUM==0");  /* kiss o' death */
#undef FAIL
	}
	printf("__LINE__:%d\n",__LINE__);
	/* XXX should I do this if debug flag is set? */
	if (ntpc->set_clock) { /* you'd better be root, or ntpclient will exit here! */
		set_time(&xmttime);
		printf("__LINE__:%d\n",__LINE__);
	}
	printf("__LINE__:%d\n",__LINE__);
	/* Not the ideal order for printing, but we want to be sure
	 * to do all the time-sensitive thinking (and time setting)
	 * before we start the output, especially fflush() (which
	 * could be slow).  Of course, if debug is turned on, speed
	 * has gone down the drain anyway. */
	if (ntpc->live) {
		int new_freq;
		new_freq = contemplate_data(arrival->coarse, (skew1-skew2)/2,
			el_time+sec2u(disp), freq);
		if (!debug && new_freq != freq) set_freq(new_freq);
	}
	printf("%d %.5d.%.3d  %8.1f %8.1f  %8.1f %8.1f %9d\n",
		arrival->coarse/86400, arrival->coarse%86400,
		arrival->fine/4294967, el_time, st_time,
		(skew1-skew2)/2, sec2u(disp), freq);
	fflush(stdout);
	*error = el_time-st_time;

	return 0;
fail:
#ifdef ENABLE_DEBUG
	printf("%d %.5d.%.3d  rejected packet: %s\n",
		arrival->coarse/86400, arrival->coarse%86400,
		arrival->fine/4294967, drop_reason);
#else
	printf("%d %.5d.%.3d  rejected packet\n",
		arrival->coarse/86400, arrival->coarse%86400,
		arrival->fine/4294967);
#endif
	return 1;
}

double ntpdiff( struct ntptime *start, struct ntptime *stop)
{
	int a;
	unsigned int b;
	a = stop->coarse - start->coarse;
	if (stop->fine >= start->fine) {
		b = stop->fine - start->fine;
	} else {
		b = start->fine - stop->fine;
		b = ~b;
		a -= 1;
	}

	return a*1.e6 + b * (1.e6/4294967296.0);
}

int get_current_freq(void)
{
	/* OS dependent routine to get the current value of clock frequency.
	 */
//#ifdef __linux__
//	struct timex txc;
//	txc.modes=0;
//	if (adjtimex(&txc) < 0) {
//		perror("adjtimex"); exit(1);
//	}
//	return txc.freq;
//#else
//	return 0;
//#endif
	return 0;
}

void set_time(struct ntptime *new)
{
	printf("__LINE__:%d\n",__LINE__);
	/* Traditional Linux way to set the system clock
	 */
	struct timeval tv_set;
	/* it would be even better to subtract half the slop */
	tv_set.tv_sec  = new->coarse - JAN_1970;
	/* divide xmttime.fine by 4294.967296 */
	tv_set.tv_usec = USEC(new->fine);
	printf("__LINE__:%d\n",__LINE__);
	if (settimeofday(&tv_set,NULL)<0) {
		perror("settimeofday");
//		exit(1);
		printf("__LINE__:%d\n",__LINE__);
	}
	if (debug) {
		printf("set time to %lu.%.6lu\n", tv_set.tv_sec, tv_set.tv_usec);
	}
}

double min_delay = 800.0;  /* global, user-changeable, units are microseconds */

#define RING_SIZE 16
#define MAX_CORRECT 250   /* ppm change to system clock */
#define MAX_C ((MAX_CORRECT)*65536)

static struct datum {
	unsigned int absolute;
	double skew;
	double errorbar;
	int freq;
	/* s.s.min and s.s.max (skews) are "corrected" to what they would
	 * have been if freq had been constant at its current value during
	 * the measurements.
	 */
	union {
		struct { double min; double max; } s;
		double ss[2];
	} s;
	/*
	double smin;
	double smax;
	 */
} d_ring[RING_SIZE];

static struct _seg {
	double slope;
	double offset;
} maxseg[RING_SIZE+1], minseg[RING_SIZE+1];

static int next_up(int i) { int r = i+1; if (r>=RING_SIZE) r=0; return r;}
static int next_dn(int i) { int r = i-1; if (r<0) r=RING_SIZE-1; return r;}
/* Looks at the line segments that start at point j, that end at
 * all following points (ending at index rp).  The initial point
 * is on curve s0, the ending point is on curve s1.  The curve choice
 * (s.min vs. s.max) is based on the index in ss[].  The scan
 * looks for the largest (sign=0) or smallest (sign=1) slope.
 */
static int search(int rp, int j, int s0, int s1, int sign, struct _seg *answer)
{
	double dt, slope;
	int n, nextj=0, cinit=1;
	for (n=next_up(j); n!=next_up(rp); n=next_up(n)) {
		if (0 && debug) printf("d_ring[%d].s.ss[%d]=%f d_ring[%d].s.ss[%d]=%f\n",
			n, s0, d_ring[n].s.ss[s0], j, s1, d_ring[j].s.ss[s1]);
		dt = d_ring[n].absolute - d_ring[j].absolute;
		slope = (d_ring[n].s.ss[s0] - d_ring[j].s.ss[s1]) / dt;
		if (0 && debug) printf("slope %d%d%d [%d,%d] = %f\n",
			s0, s1, sign, j, n, slope);
		if (cinit || (slope < answer->slope) ^ sign) {
			answer->slope = slope;
			answer->offset = d_ring[n].s.ss[s0] +
				slope*(d_ring[rp].absolute - d_ring[n].absolute);
			cinit = 0;
			nextj = n;
		}
	}
	return nextj;
}

/* Pseudo-class for finding consistent frequency shift */
#define MIN_INIT 20
static struct _polygon {
	double l_min;
	double r_min;
} df;

static void polygon_reset(void)
{
	df.l_min = MIN_INIT;
	df.r_min = MIN_INIT;
}

static double find_df(int *flag)
{
	if (df.l_min == 0.0) {
		if (df.r_min == 0.0) {
			return 0.0;   /* every point was OK */
		} else {
			return -df.r_min;
		}
	} else {
		if (df.r_min == 0.0) {
			return df.l_min;
		} else {
			if (flag) *flag=1;
			return 0.0;   /* some points on each side,
			               * or no data at all */
		}
	}
}

/* Finds the amount of delta-f required to move a point onto a
 * target line in delta-f/delta-t phase space.  Any line is OK
 * as long as it's not convex and never returns greater than
 * MIN_INIT. */
static double find_shift(double slope, double offset)
{
	double shift  = slope - offset/600.0;
	double shift2 = slope + 0.3 - offset/6000.0;
	if (shift2 < shift) shift = shift2;
	if (debug) printf("find_shift %f %f -> %f\n", slope, offset, shift);
	if (shift  < 0) return 0.0;
	return shift;
}

void polygon_point(struct _seg *s)
{
	double l, r;
	if (debug) printf("loop %f %f\n", s->slope, s->offset);
	l = find_shift(- s->slope,   s->offset);
	r = find_shift(  s->slope, - s->offset);
	if (l < df.l_min) df.l_min = l;
	if (r < df.r_min) df.r_min = r;
	if (debug) printf("constraint left:  %f %f \n", l, df.l_min);
	if (debug) printf("constraint right: %f %f \n", r, df.r_min);
}

/* Something like linear feedback to be used when we are "close" to
 * phase lock.  Not really used at the moment:  the logic in find_df()
 * never sets the flag. */
static double find_df_center(struct _seg *min, struct _seg *max, double gross_df)
{
	const double crit_time=1000.0;
	double slope  = 0.5 * (max->slope  + min->slope)+gross_df;
	double dslope =       (max->slope  - min->slope);
	double offset = 0.5 * (max->offset + min->offset);
	double doffset =      (max->offset - min->offset);
	double delta1 = -offset/ 600.0 - slope;
	double delta2 = -offset/1800.0 - slope;
	double delta  = 0.0;
	double factor = crit_time/(crit_time+doffset+dslope*1200.0);
	if (offset <  0 && delta2 > 0) delta = delta2;
	if (offset <  0 && delta1 < 0) delta = delta1;
	if (offset >= 0 && delta1 > 0) delta = delta1;
	if (offset >= 0 && delta2 < 0) delta = delta2;
	if (max->offset < -crit_time || min->offset > crit_time) return 0.0;
	if (debug) printf("find_df_center %f %f\n", delta, factor);
	return factor*delta;
}

int contemplate_data(unsigned int absolute, double skew, double errorbar, int freq)
{
	/*  Here is the actual phase lock loop.
	 *  Need to keep a ring buffer of points to make a rational
	 *  decision how to proceed.  if (debug) print a lot.
	 */
	static int rp=0, valid=0;
	int both_sides_now=0;
	int j, n, c, max_avail, min_avail, dinit;
	int nextj=0;	/* initialization not needed; but gcc can't figure out my logic */
	double cum;
	struct _seg check, save_min, save_max;
	double last_slope;
	int delta_freq;
	double delta_f;
	int inconsistent=0, max_imax, max_imin=0, min_imax, min_imin=0;
	int computed_freq=freq;

	if (debug) printf("xontemplate %u %.1f %.1f %d\n",absolute,skew,errorbar,freq);
	d_ring[rp].absolute = absolute;
	d_ring[rp].skew     = skew;
	d_ring[rp].errorbar = errorbar - min_delay;   /* quick hack to speed things up */
	d_ring[rp].freq     = freq;

	if (valid<RING_SIZE) ++valid;
	if (valid==RING_SIZE) {
		/*
		 * Pass 1: correct for wandering freq's */
		cum = 0.0;
		if (debug) printf("\n");
		for (j=rp; ; j=n) {
			d_ring[j].s.s.max = d_ring[j].skew - cum + d_ring[j].errorbar;
			d_ring[j].s.s.min = d_ring[j].skew - cum - d_ring[j].errorbar;
			if (debug) printf("hist %d %d %f %f %f\n",j,d_ring[j].absolute-absolute,
				cum,d_ring[j].s.s.min,d_ring[j].s.s.max);
			n=next_dn(j);
			if (n == rp) break;
			/* Assume the freq change took place immediately after
			 * the data was taken; this is valid for the case where
			 * this program was responsible for the change.
			 */
			cum = cum + (d_ring[j].absolute-d_ring[n].absolute) *
				(double)(d_ring[j].freq-freq)/65536;
		}
		/*
		 * Pass 2: find the convex down envelope of s.max, composed of
		 * line segments in s.max vs. absolute space, which are
		 * points in freq vs. dt space.  Find points in order of increasing
		 * slope == freq */
		dinit=1; last_slope=-2*MAX_CORRECT;
		for (c=1, j=next_up(rp); ; j=nextj) {
			nextj = search(rp, j, 1, 1, 0, &maxseg[c]);
			        search(rp, j, 0, 1, 1, &check);
			if (check.slope < maxseg[c].slope && check.slope > last_slope &&
			    (dinit || check.slope < save_min.slope)) {dinit=0; save_min=check; }
			if (debug) printf("maxseg[%d] = %f *x+ %f\n",
				 c, maxseg[c].slope, maxseg[c].offset);
			last_slope = maxseg[c].slope;
			c++;
			if (nextj == rp) break;
		}
		if (dinit==1) inconsistent=1;
		if (debug && dinit==0) printf ("mincross %f *x+ %f\n", save_min.slope, save_min.offset);
		max_avail=c;
		/*
		 * Pass 3: find the convex up envelope of s.min, composed of
		 * line segments in s.min vs. absolute space, which are
		 * points in freq vs. dt space.  These points are found in
		 * order of decreasing slope. */
		dinit=1; last_slope=+2*MAX_CORRECT;
		for (c=1, j=next_up(rp); ; j=nextj) {
			nextj = search(rp, j, 0, 0, 1, &minseg[c]);
			        search(rp, j, 1, 0, 0, &check);
			if (check.slope > minseg[c].slope && check.slope < last_slope &&
			    (dinit || check.slope < save_max.slope)) {dinit=0; save_max=check; }
			if (debug) printf("minseg[%d] = %f *x+ %f\n",
				 c, minseg[c].slope, minseg[c].offset);
			last_slope = minseg[c].slope;
			c++;
			if (nextj == rp) break;
		}
		if (dinit==1) inconsistent=1;
		if (debug && dinit==0) printf ("maxcross %f *x+ %f\n", save_max.slope, save_max.offset);
		min_avail=c;
		/*
		 * Pass 4: splice together the convex polygon that forms
		 * the envelope of slope/offset coordinates that are consistent
		 * with the observed data.  The order of calls to polygon_point
		 * doesn't matter for the frequency shift determination, but
		 * the order chosen is nice for visual display. */
		if (!inconsistent) {
		polygon_reset();
		polygon_point(&save_min);
		for (dinit=1, c=1; c<max_avail; c++) {
			if (dinit && maxseg[c].slope > save_min.slope) {
				max_imin = c-1;
				maxseg[max_imin] = save_min;
				dinit = 0;
			}
			if (maxseg[c].slope > save_max.slope)
				break;
			if (dinit==0) polygon_point(&maxseg[c]);
		}
		if (dinit && debug) printf("found maxseg vs. save_min inconsistency\n");
		if (dinit) inconsistent=1;
		max_imax = c;
		maxseg[max_imax] = save_max;

		polygon_point(&save_max);
		for (dinit=1, c=1; c<min_avail; c++) {
			if (dinit && minseg[c].slope < save_max.slope) {
				max_imin = c-1;
				minseg[min_imin] = save_max;
				dinit = 0;
			}
			if (minseg[c].slope < save_min.slope)
				break;
			if (dinit==0) polygon_point(&minseg[c]);
		}
		if (dinit && debug) printf("found minseg vs. save_max inconsistency\n");
		if (dinit) inconsistent=1;
		min_imax = c;
		minseg[min_imax] = save_max;

		/* not needed for analysis, but shouldn't hurt either */
		if (debug) polygon_point(&save_min);
		} /* !inconsistent */

		/*
		 * Pass 5: decide on a new freq */
		if (inconsistent) {
			printf("# inconsistent\n");
		} else {
			delta_f = find_df(&both_sides_now);
			if (debug) printf("find_df() = %e\n", delta_f);
			delta_f += find_df_center(&save_min,&save_max, delta_f);
			delta_freq = delta_f*65536+.5;
			if (debug) printf("delta_f %f  delta_freq %d  bsn %d\n", delta_f, delta_freq, both_sides_now);
			computed_freq -= delta_freq;
			printf ("# box [( %.3f , %.1f ) ",  save_min.slope, save_min.offset);
			printf (      " ( %.3f , %.1f )] ", save_max.slope, save_max.offset);
			printf (" delta_f %.3f  computed_freq %d\n", delta_f, computed_freq);

			if (computed_freq < -MAX_C) computed_freq=-MAX_C;
			if (computed_freq >  MAX_C) computed_freq= MAX_C;
		}
	}
	rp = (rp+1)%RING_SIZE;
	return computed_freq;
}

int set_freq(int new_freq)
{
	/* OS dependent routine to set a new value of clock frequency.
	 */
//#ifdef __linux__
//	struct timex txc;
//	txc.modes = ADJ_FREQUENCY;
//	txc.freq = new_freq;
//	if (adjtimex(&txc) < 0) {
//		perror("adjtimex"); exit(1);
//	}
//	return txc.freq;
//#else
//	return 0;
//#endif
	return 0;
}

#ifdef _IMG_TYPE_BELL
#define time_24H 1*60*60
#else
#define time_24H 24*60*60
#endif
int get_after_NTP_time(long int T0){

	time_t t1;
	long int T1;

	T1 = time(&t1);

	if(T1-T0 >= time_24H){
		time_flag = 1;
	}
	return 0;
}
