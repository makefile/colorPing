/*本地回环测试时本机发送ECHO也就收到ECHO，同时也收到自己发给自己的ECHOREPLY,因此需要在接受函数中仔细判别*/
#include<stdio.h>
#include<stdlib.h>
#include<string.h>
//#include<signal.h>
#include<arpa/inet.h>
#include<sys/types.h>
#include<sys/socket.h>
#include<unistd.h>
#include<netdb.h>
#include<errno.h>
#include<pthread.h>

#include"util.h"
int nsend=0,nreceived=0,max_no_packets=4;
//这是全局变量，在util.c中通过extern来跨文件使用，不要将普通变量放在头文件中，会导致重复定义，即使放到ifndef中也不行，但似乎结构体等类型可以。
int tmin=10000,tmax=0;//round trip time
unsigned int tsum=0;//for average
void printUsage();
int main(int argc,char **argv){
	int sockfd;
	struct sockaddr_in dest_addr;
	pid_t pid;
	struct hostent *host;
	struct protoent *protocol;
	unsigned long inaddr=0l;
//	int waittime=MAX_WAIT_TIME;
	int size=5120;//50*1024
	int result;
	while((result=getopt(argc,argv,"c:"))!=-1){
		switch(result){
			case 'c':
				if(optarg==NULL||
						(max_no_packets=atoi(optarg))==-1){
					printf("please specify the number\n");
					printUsage();
					exit(1);
				}
		//	default:printUsage();
		//			exit(1);
		}
	}
	argc-=optind;
	argv+=optind;
	if(argc!=1){
		printUsage();
		exit(1);
	}
	if((protocol=getprotobyname("icmp"))==NULL){
		perror("getprotobyname");
		exit(1);
	}
	if((sockfd=socket(AF_INET,SOCK_RAW,protocol->p_proto))<0){
		//am_i_root=(getuid()==0);
		if(errno==EPERM)
			fprintf(stderr,"Ping:ping must run as root\n");
		else{
			perror("socket error");
			exit(2);
		}
	}
	/* 回收root权限,设置当前用户权限*/
	setuid(getuid());//s flag
	/*扩大套接字接收缓冲区到50K这样做主要为了减小接收缓冲区溢出的可能性,若无意中ping一个广播地址或多播地址,将会引来大量应答*/
	setsockopt(sockfd,SOL_SOCKET,SO_RCVBUF,&size,sizeof(size));//modify the recv buffer
	bzero(&dest_addr,sizeof(dest_addr));
	dest_addr.sin_family=AF_INET;
	if((inaddr=inet_addr(argv[0]))==INADDR_NONE){
		if((host=gethostbyname(argv[0]))==NULL){
			perror("gethostbyname");
			exit(1);
		}
		memcpy((char*)&(dest_addr.sin_addr),host->h_addr,host->h_length);
	}else{
		memcpy((char*)&(dest_addr.sin_addr),(char*)&inaddr,sizeof(inaddr));
	}
	
	printf("PING %s(%s): %d bytes of data in ICMP packets.\n",argv[0],inet_ntoa(dest_addr.sin_addr),DATA_LEN);

	pid=getpid();//获取当前进程ID，作为多个ICMP进程的区分
	//pthread_t thd;
	struct param p={sockfd,pid,dest_addr};
	//int ret=pthread_create(&thd,NULL,(void*)send_packet,&p);
	send_packet(&p);
	signal(SIGINT,statistics);
	//捕捉中断信号，ctrl+c产生SIGINT，立刻统计百分比,注意安装信号要放在join等待之前，开始发包之后，否则不显示统计结果
/*	if(pthread_join(thd,NULL)!=0){//阻塞当前线程，直到新线程执行完毕，但这未达目的，需修改recv_packet
		perror("join");
		exit(errno);
	}
	else printf("join normal\n");
*/
//	recv_packet(sockfd,pid);
	//statistics(SIGCONT);//此处在收包完毕后显式调用，另外在recv_packet()中超时时也会调用，调用一次就退出程序。此处随便传入一个信号即可，alarm,continue,quit等
	close(sockfd);
	return 0;
}
void printUsage(){
		printf("usage:myPing hostname/IP\n"
				"	-c number:set the number\n"
				"	   of packet you want to\n"
			    "		send,the default is 4\n");
}
