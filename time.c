#include<sys/time.h>//not same to time.h
#include<stdio.h>
int main(){
	struct timeval *tv;
	struct timezone *tz;
	gettimeofday(tv,tz);
	printf("sec=%lld,usec=%lld\n"
			"tz_minuteswest:%lld "
			"tz_dsttime:%lld\n",tv->tv_sec,tv->tv_usec,
			tz->tz_minuteswest,tz->tz_dsttime);
}
