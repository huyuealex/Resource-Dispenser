#include <sys/types.h>
#include <sys/stat.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <fcntl.h>
#include <sys/time.h>
#include <time.h>
#include <unistd.h>
#include <sys/sem.h>
#include <sys/ipc.h>
#include <sys/shm.h>

#define BUF_SIZE 2048
//assume
#define PAGE_SIZE 4096
#define INTERVAL 10
#define MSGS 10000

char file_priority[6][2][50] = 
{
	{"file1.txt", "file7.txt"},
	{"file2.txt", "file8.txt"},
	{"file3.txt", "file9.txt"},
	{"file4.txt", "file10.txt"},
	{"file5.txt", "file11.txt"},
	{"file6.txt", "file12.txt"}
};

char shell_cmd[500];
char file[100];

int segment_id;
typedef struct{  
	char msg[MSGS][100];  
	int cnt;  
}shared_msgs;

char output[MSGS * 100];
char cmd[MSGS * 100];

int main(int argc, char *argv[])
{
	/*
	 * 分配日志需要的进程共享内存
	 */
	const int SIZE = sizeof(shared_msgs);  
	segment_id = shmget(IPC_PRIVATE, SIZE, S_IRUSR | S_IWUSR);
	shared_msgs *msgs= (shared_msgs*) shmat(segment_id, NULL, 0);
	msgs->cnt = 0;
	/*
	 * 建立互斥量
	 */
	int semid;
	static struct sembuf lock={0,-1,SEM_UNDO};
	static struct sembuf unlock={0,1,SEM_UNDO|IPC_NOWAIT};
	if((semid = semget(998, 1, IPC_CREAT | IPC_EXCL | 0666)) == -1)
	{
		printf("Error: semget!\n");
		return 1;
	}
	if(semctl(semid, 0, SETVAL, 1) == -1)
	{
		printf("Error: semctl!\n");
		return 1;
	}
	/*
	 * 生成多进程
	 */
	pid_t fpid;
	int i, j, prio;
	time_t deadline = (time_t)((int)time(NULL) + 2);
	for (i = 0; i < 12; i++)
	{
		fpid = fork();
		if (fpid == 0)
		{
			sprintf(file, "file%d.txt", i + 1);
			break;
		}
	}
	if (fpid > 0) {
		for (i = 0; i < 12; i++)
			wait(NULL);
		system("echo 'clear' > /proc/rdnice");

		if(semctl(semid, 0, IPC_RMID, 0) == -1) // 删除互斥量
		{
			printf("error: semctl!\n");
			return 1;
		}
		/*
		 * 将日志输出至文件
		 */
		int pos = 0, len = 0;
		for (i = 0; i < msgs->cnt; i++)
		{
			len = sprintf(output + pos, "%s", msgs->msg[i]);
			pos += len;
		}
		sprintf(cmd, "echo '%s' > log", output);
		system(cmd);
		printf("Done.\n");
		return 0;
	}
	fpid = getpid();
	for (i = 0; i < 6; i++)
		for (j = 0; j < 2; j++)
			if (strcmp(file, file_priority[i][j]) == 0)
				prio = i;
	sprintf(shell_cmd, "echo 'add %d %d' > /proc/rdnice", fpid, prio);
	//printf("%s\n", shell_cmd);	
	system(shell_cmd);
	while (time(NULL) < deadline); // 等等慢的进程，大家一起来读文件

	if((semid = semget(998, 1, 0)) == -1) // 读取互斥量
	{
		printf("Error: semget!\n");
		return 1;
	}

	int fd, intervalPercent = INTERVAL;
	struct timeval sTime, eTime;
	unsigned intervalByte, nextCheckPoint;
	char buf[BUF_SIZE];

	fd = open(file, O_RDONLY);
	if(fd == -1)
	{
		perror("can't open the file\n");
		return 1;
	}
	// free the file's cache
	if(posix_fadvise(fd, 0, 0, POSIX_FADV_DONTNEED) != 0)
	{
		perror("free cache failed\n");
		return 1;
	}

	struct stat fdstat;
	fstat(fd, &fdstat);
	nextCheckPoint = intervalByte = (unsigned)(fdstat.st_size * (intervalPercent / 100.0));
	ssize_t currentRead = BUF_SIZE, totalRead=0;
	gettimeofday(&sTime, NULL);
	while(currentRead == BUF_SIZE)
	{
		if(totalRead >= nextCheckPoint)
		{
			if(semop(semid, &lock, 1) == -1)
			{
				printf("Error: semop(lock)!\n");
				return 1;
			}
			sprintf(msgs->msg[msgs->cnt], "(pid = %d, prio = %d): %.1f%%\n", fpid, prio, (double)(totalRead) / fdstat.st_size*100);
			msgs->cnt++;
			if(semop(semid, &unlock, 1) == -1)
			{
				printf("Error: semop(unlock)!\n");
				return 1;
			}
			nextCheckPoint += intervalByte;
		}
		if((currentRead = read(fd, buf, sizeof(buf)))>0)
			totalRead += currentRead;
		buf[2] = buf[1026] = 2;
	}
	gettimeofday(&eTime, NULL);
	if(semop(semid, &lock, 1) == -1)
	{
		printf("Error: semop(lock)!\n");
		return 1;
	}
	sprintf(msgs->msg[msgs->cnt], "Process %d finished. (File: %s, Read: %ziB, Time: %lims)\n",
			fpid,
			file,
			totalRead,
			(eTime.tv_sec-sTime.tv_sec)*1000+(eTime.tv_usec-sTime.tv_usec)/1000);
	msgs->cnt++;
	if(semop(semid, &unlock, 1) == -1)
	{
		printf("Error: semop(unlock)!\n");
		return 1;
	}
	close(fd);
	return 0;
}
