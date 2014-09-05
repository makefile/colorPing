#include"util.h"
extern int nsend,nreceived,max_no_packets;
extern int tmax,tmin;
extern unsigned int tsum;
struct timeval tvrecv;
/*两种计算时间的方式：接受时的时间都按gettimeofday,而发送时间一种将发送时间作为全局或共享的，另一种就是在发送包的数据区域（最大65535b，即64k）中填充时间，从接受到的响应包中读请求包的发送时间，因为响应包会复制请求包的数据区域中的所有数据。因此可以在数据区域填充MAC地址等*/
void send_packet(struct param *p){
	//参数使用结构体指针是因为如果把此函数作为新线程的执行函数，传入多个参数时只能是结构体指针，但不能是结构体。
	int sockfd=p->fd;
	int pid=p->id;
	//printf("sockfd=%d,pid=%d\n",sockfd,pid);
	struct sockaddr_in dest_addr=p->dest;
	int packetsize;//,nsend=0;
	while(nsend<max_no_packets){
		//printf("nsend:%d no_packets:%d\n",nsend,max_no_packets);
		nsend++;
		packetsize=pack(nsend,pid);//以当前进程PID封装数据包
		//gettimeofday(&tvsend,(struct timezone*)NULL);
		if(sendto(sockfd,sendpacket,packetsize,0,(struct sockaddr *)&dest_addr,sizeof(dest_addr))<0){
			perror("sendto");
			continue;
		}
		//不知道这时候是否能够手动刷新缓存
		recv_packet(sockfd,pid);
		sleep(1);//1s,notice the sleep must be after recv_packet
	}
	statistics(SIGCONT);
}
int pack(int pack_no,int pid){
	int packsize;
	struct icmp *_icmp;
	struct timeval *tval;
//	struct timezone *tz;
//	memset(_icmp,0,sizeof(struct icmp));//this and the follow line could cause the program terminate,because i did't assign space for *_icmp,also there is no need,for we have bzero sendpacket in the following
//	bzero(icmp,sizeof(struct icmp));
	memset(sendpacket,0,sizeof(sendpacket));//seems no need
	_icmp=(struct icmp*)sendpacket;//封装包头，sendpacket为char []
	_icmp->icmp_type=ICMP_ECHO;//协议
	_icmp->icmp_code=0;//ECHO
	//_icmp->icmp_cksum=0;
	_icmp->icmp_seq=pack_no;//包序号
	_icmp->icmp_id=pid;//区分同主机多个ICMP
	packsize=8+DATA_LEN;//8 is the len of head
	tval=(struct timeval*)_icmp->icmp_data;//以当前时间为数据，用于计算延迟,现在获得指针，下面一句对其赋值
//	puts("in pack:here2");
	gettimeofday(tval,(struct timezone*)NULL);//将当前时间赋给tval,当前时区赋值给第二个参数,第二个参数可以是NULL
	//printf("in pack:tval=%llds,%lldus\n",tval->tv_sec,tval->tv_usec);
	_icmp->icmp_cksum=cal_chksum((unsigned short *)_icmp,packsize);
	return packsize;
}
unsigned short cal_chksum(unsigned short *addr,int len){
	int nleft=len;
	int sum=0;
	unsigned short *w=addr;
	unsigned short answer=0;
	/*把ICMP报头二进制数据以2字节为单位累加起来*/
	while(nleft>1){
		sum+=*w++;
		nleft-=2;
	}
	/*若ICMP报头为奇数个字节，会剩下最后一字节。把最后一个字节视为一个2字节数据的高字节，这个2字节数据的低字节为0，继续累加*/
	if(nleft==1){
		*(unsigned char*)(&answer)=*(unsigned char*)w;
		sum+=answer;
	}
	sum=(sum>>16)+(sum&0xffff);
	sum+=(sum>>16);
	answer=~sum;
	return answer;
}
void recv_packet(int sockfd,int pid){
	int n;
	unsigned int fromlen;
	//extern int errno;
	//extern struct timeval tvrecv;
	signal(SIGALRM,statistics);//安装信号，到指定时间就计算，无论收到多少包
	fromlen=sizeof(from);
	while(nreceived<nsend){//本程序的思路是send_packet中每发一个包立即用此函数接收，因此正常情况这里只循环一次，也没必要写成循环，但还考虑到以后的逻辑更改。
		alarm(MAX_WAIT_TIME);//第一次执行设置等待时间发送SIGALRM信号，第二次执行重置时间，但返回上次设置的剩余时间，
		//此处即每个包的接收超时时间都是5s，自定义的秒数
		if((n=recvfrom(sockfd,recvpacket,sizeof(recvpacket),0,(struct sockaddr*)&from,&fromlen))<0){
			if(errno==EINTR) continue;
			perror("recvfrom");
			continue;
		}
	//	gettimeofday(&tvrecv,NULL);//global val:tvrecv
		//printf("in recv_packet:%lld %lld\n",tvrecv.tv_sec,tvrecv.tv_usec);
		if(unpack(recvpacket,n,pid)==-1) {
			//fprintf(stderr,"unpack fail\n");
			continue;//解包
		}
		nreceived++;
	}
}
int unpack(char *buf,int len,int pid){
	int iphdrlen;
	struct ip *ip;
	struct icmp *icmp;
	struct timeval *tvsend;
	double rtt;
	ip=(struct ip*)buf;//header
	iphdrlen=ip->ip_hl<<2;//why?
	icmp=(struct icmp*)(buf+iphdrlen);//头部之后的数据位置
	len-=iphdrlen;
	if(len<8){//data len<8 bytes,error
		fprintf(stderr,"ICMP packet\'s length is less than 8\n");
		return -1;
	}
	//ICMP_ECHO=0,ICMP_ECHOREPLY=8
	if((icmp->icmp_type==ICMP_ECHOREPLY)&&(icmp->icmp_id==pid)){//确保是回复包，且是回复给本进程的
		//我了个去，感觉被书上给误导了，主机在自动回复应答包时有没有填充时间？时间应该填在哪个地方？
		tvsend=(struct timeval*)icmp->icmp_data;//获取此数据包发送时间，因为数据中只有时间。
		//printf("in unpack:tvsend:%lld %lld\n",tvsend->tv_sec,tvsend->tv_usec);
		gettimeofday(&tvrecv,NULL);//global val:tvrecv
		tv_sub(&tvrecv,tvsend);//计算发送与接受差值,存于tvrecv
		rtt=(tvrecv.tv_sec)*1000+tvrecv.tv_usec/1000;
		//差值 毫秒
		printf("\033[32;40m%d byte from %s: icmp_seq=%u ttl=%d time=\033[0m\033[34;42m%.1f\033[0m ms\n",len,inet_ntoa(from.sin_addr),icmp->icmp_seq,ip->ip_ttl,rtt);
		tsum+=rtt;
		if(rtt<=tmin) tmin=rtt;
		else if(rtt>tmax) tmax=rtt;
	}
	else if (icmp->icmp_type==ICMP_DEST_UNREACH){
		printf("icmp_seq=%d,host unreachable\n",icmp->icmp_seq);
	
//		printf("icmp_type:%d,expect:%d id:%d\n",icmp->icmp_type,ICMP_ECHOREPLY,icmp->icmp_id);
	}
	return 0;
}
void tv_sub(struct timeval *rec,struct timeval *sent){
	if((rec->tv_usec-=sent->tv_usec)<0){
		--rec->tv_sec;
		rec->tv_usec+=1000000;
	}
	rec->tv_sec-=sent->tv_sec;
}
void statistics(int signo){
	(void)signal(SIGINT, SIG_IGN);//prevent from user interupt,when counting
	putchar('\n');
	fflush(stdout);
	printf("\033[35;40m---- PING statistics ----\033[0m\n");
	//40~47=30~37:黑 红 绿 黄 蓝 紫 深绿 白
	//\033[foreground;background
	printf("\033[32;40m%d packets transmitted, %d received, %.2f%% loss\033[0m\n",nsend,nreceived,(nsend-nreceived)*1.0/nsend*100);
	printf("\033[36;40mrtt min/avg/max = %.2lf/%.2lf/%.2lf ms\033[0m\n",//loss mdev
			(double)tmin,tsum/(double)nreceived,(double)tmax);//rtt:round trip time
	if(signo==SIGALRM||signo==SIGINT) exit(1);
	//超时或用户中断，统计并退出

	exit(0);
}


