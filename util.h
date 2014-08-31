#ifndef UTIL_H
#define UTIL_H
#include<stdio.h>
#include<stdlib.h>
#include<string.h>
#include<signal.h>
#include<sys/time.h>//struct timeval,gettimeofday()
#include<arpa/inet.h>
#include<sys/types.h>
#include<sys/socket.h>
#include<unistd.h>
#include<netinet/in.h>
#include<netinet/ip_icmp.h>
#include<netdb.h>
#include<setjmp.h>
#include<errno.h>
#define PACKET_SIZE 4096//4k
#define MAX_WAIT_TIME 5
//#define MAX_NO_PACKETS 4
#define DATA_LEN 56
	struct param{
		int fd;
		int id;
		struct sockaddr_in dest;
	};
char sendpacket[PACKET_SIZE];
char recvpacket[PACKET_SIZE];
struct timeval tvrecv;
struct sockaddr_in from;
extern void statistics(int signo);
extern unsigned short cal_chksum(unsigned short *addr,int len);
extern int pack(int pack_no,int pid);
extern void send_packet(struct param *);
extern void recv_packet(int sockfd,int pid);
extern int unpack(char*,int len,int pid);
extern void tv_sub(struct timeval *out,struct timeval *in);
#endif
