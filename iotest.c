#include <sys/types.h>
#include <sys/stat.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <fcntl.h>
#include <sys/time.h>
#include <time.h>

#define BUF_SIZE 2048

//assume
#define PAGE_SIZE 4096

int main(int argc, char *argv[])
{
	int c,fd,intervalPercent=100;
	struct timeval sTime,eTime;
	unsigned intervalByte,nextCheckPoint;
	char *file=0,buf[BUF_SIZE];

	//确保缓冲区的内存页面已经被分配
	int i=0;
	while(i<BUF_SIZE)
	{
		buf[i]=0;
		i+=PAGE_SIZE;
	}
	while ((c = getopt(argc, argv, "f:v:h")) != EOF)
	{
		switch(c)
		{
			case 'f':
				file=optarg;
				break;
			case 'v':
				intervalPercent=strtol(optarg, NULL, 10);
				break;
			case 'h':
			case '?':
				printf("usage:\tiotest [-f <file>] [-v <interval>]\n");
				return 0;
		}
	}
	if(file==0)
		file="iotest_source";
	fd=open(file,O_RDONLY);
	if(fd==-1)
	{
		perror("can't open the file\n");
		return 1;
	}
	/*
	 * free the file's cache
	 */
	if(posix_fadvise(fd,0,0,POSIX_FADV_DONTNEED)!=0)
	{
		perror("free cache failed\n");
		return 1;
	}
	struct stat fdstat;
	fstat(fd,&fdstat);
	//printf("file size:%lukb\n",fdstat.st_size);
	if(intervalPercent>100||intervalPercent<=0)
		intervalPercent=100;
	nextCheckPoint=intervalByte=(unsigned)(fdstat.st_size*(intervalPercent/100.));
	ssize_t currentRead=BUF_SIZE,totalRead=0;
	gettimeofday(&sTime,NULL);
	while(currentRead==BUF_SIZE)
	{
		if(totalRead>=nextCheckPoint)
		{
			printf("%.1f%%\n",(double)(totalRead)/fdstat.st_size*100);
			nextCheckPoint+=intervalByte;
		}
		if((currentRead=read(fd,buf,sizeof(buf)))>0)
			totalRead+=currentRead;
		buf[2]=buf[1026]=2;
	}
	gettimeofday(&eTime,NULL);
	printf("finished\nRead Byte:%zi\tRead times:%lims\n",
			totalRead,
			(eTime.tv_sec-sTime.tv_sec)*1000+(eTime.tv_usec-sTime.tv_usec)/1000);
	close(fd);
	return 0;
}
