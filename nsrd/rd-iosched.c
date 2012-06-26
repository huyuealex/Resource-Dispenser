/*
 * elevator nsrd(not-simple-rd)
 */
#include <linux/blkdev.h>
#include <linux/elevator.h>
#include <linux/bio.h>
#include <linux/module.h>
#include <linux/init.h>
#include <linux/proc_fs.h>
#include <linux/kernel.h>
#include <linux/random.h>
//#include <linux/jiffies.h>
#define MOD 			100003
#define LEN 			10
#define MAXN_PRIO 		5
#define TIME_AFTER_DISPATCH	20
#define TIME_RESET		30
#define DEN			10007

static char proc_writebuf[100];
static char proc_readbuf[1000];
static struct proc_dir_entry *proc_entry;
static unsigned long dontuntil = 0; // 只有当jiffies>dontuntil后才能dispatch普通进程

static unsigned cnt_processes[MAXN_PRIO + 1];
static int last_prio = 0;

static int threshold = 0; // 用来判断使用随机法还是优先级法的阀值
static int random = 0; // 1为使用随机法，0为使用优先级法
static int rand_yes = 0, rand_no = 0; // 测试用

struct io_process {
	pid_t pid; // 进程id
	int prio; // 优先级：0-5(MAXN_PRIO)，5为最高，0为最低
	int alive; // 1为活跃，0为死亡
};

static struct io_process hash_process[MOD][LEN]; // 用来保存进程优先级的hash

struct rd_data {
	struct request_queue *q;
	struct timer_list resume_timer; // 用来重新唤醒io
	struct list_head prio_queue[MAXN_PRIO + 1]; // 根据不同优先级的队列，未定义优先级的进程默认为0，即优先级最低
}*ndglobal;

//FIXME
static void rd_merged_requests(struct request_queue *q, struct request *rq,
		struct request *next)
{
	list_del_init(&next->queuelist);
}

static int rd_dispatch(struct request_queue *q, int force)
{
	struct rd_data *nd = q->elevator->elevator_data;
	int i = 0, result = 0;
	unsigned int randNum;
	int goodluck;
	int s, e;
	if (random) {
		get_random_bytes(&randNum, sizeof(unsigned int));
		if (randNum < 0) randNum *= -1;
		s = randNum % (MAXN_PRIO + 1);
		e = (s + 1) % (MAXN_PRIO + 1);
		for (i = s; ; i = (i - 1 + (MAXN_PRIO + 1)) % (MAXN_PRIO + 1)) {
			if (!list_empty(&nd->prio_queue[i])) {
				struct request *rq;
				rq = list_entry(nd->prio_queue[i].next, struct request, queuelist);
				list_del_init(&rq->queuelist);
				elv_dispatch_sort(q, rq);
				cnt_processes[i]++;
				dontuntil = jiffies + msecs_to_jiffies(TIME_AFTER_DISPATCH);
				mod_timer(&nd->resume_timer, jiffies + msecs_to_jiffies(TIME_RESET));
				last_prio = i;
				result += 1;
				break;
			}
			if (i == e) break;
		}
		//if (result == 0) random = 0;
	}
	if (!random) {
		for (i = MAXN_PRIO; i >= 0; i--) {
			if (!list_empty(&nd->prio_queue[i])) {
				if (last_prio <= i || force || time_after(jiffies, dontuntil)) {
					struct request *rq;
					rq = list_entry(nd->prio_queue[i].next, struct request, queuelist);
					list_del_init(&rq->queuelist);
					elv_dispatch_sort(q, rq);
					cnt_processes[i]++;
					dontuntil = jiffies + msecs_to_jiffies(TIME_AFTER_DISPATCH);
					mod_timer(&nd->resume_timer, jiffies + msecs_to_jiffies(TIME_RESET));
					last_prio = i;
					result += 1;
					break;
				}
			}
		}
	}
	get_random_bytes(&randNum, sizeof(unsigned int));
	if (randNum < 0) randNum *= -1;
	goodluck = randNum % DEN;
	if (goodluck == 0) goodluck++;
	if (goodluck <= threshold) random = 1;
	else random = 0;
	if (random) rand_yes++;
	else rand_no++;
	return result;
}

static void rd_add_request(struct request_queue *q, struct request *rq)
{
	struct rd_data *nd = q->elevator->elevator_data;
	int key = current->pid % MOD;
	int i = 0, prio = 0;

	for (i = 0; i < LEN; i++) {
		if (hash_process[key][i].alive == 1 && current->pid == hash_process[key][i].pid) {
			prio = hash_process[key][i].prio;
			break;
		}
	}
	list_add_tail(&rq->queuelist, &nd->prio_queue[prio]);
}

static int rd_queue_empty(struct request_queue *q)
{
	struct rd_data *nd = q->elevator->elevator_data;
	int i = 0;
	if (random) {
		for (i = MAXN_PRIO; i >= 0; i--) {
			if (!list_empty(&nd->prio_queue[i]))
				return 0;
		}
		return 1;
	}
	for (i = MAXN_PRIO; i >= last_prio; i--) {
		if (!list_empty(&nd->prio_queue[i]))
			return 0;
	}
	if (!time_after(jiffies, dontuntil)) return 1;
	for (i = last_prio - 1; i >= 0; i--) {
		if (!list_empty(&nd->prio_queue[i]))
			return 0;
	}
	return 1;
}

static struct request *rd_former_request(struct request_queue *q, struct request *rq)
{
	struct rd_data *nd = q->elevator->elevator_data;
	int i = 0;
	for (i = 0; i <= MAXN_PRIO; i++) {
		if (rq->queuelist.prev == &nd->prio_queue[i])
			return NULL;
	}
	return list_entry(rq->queuelist.prev, struct request, queuelist);
}

static struct request *rd_latter_request(struct request_queue *q, struct request *rq)
{
	struct rd_data *nd = q->elevator->elevator_data;
	int i = 0;
	for (i = 0; i <= MAXN_PRIO; i++) {
		if (rq->queuelist.next == &nd->prio_queue[i])
			return NULL;
	}
	return list_entry(rq->queuelist.next, struct request, queuelist);
}

static void resume_timeout(unsigned long data)
{
	//int i = 0;
	//printk(KERN_INFO "0-timeout, j = %lu",jiffies);
	//for (i = 0; i <= MAXN_PRIO; i++)
	//	printk(KERN_INFO ", cnt_prio_%d = %u", i, cnt_processes[i]);
	//printk(KERN_INFO "\n");
	blk_run_queue((struct request_queue *)data);
	//printk(KERN_INFO "1-timeout, j = %lu",jiffies);
	//for (i = 0; i <= MAXN_PRIO; i++)
	//	printk(KERN_INFO ", cnt_prio_%d = %u", i, cnt_processes[i]);
	//printk(KERN_INFO "\n");
}

static void clear_hash(void)
{
	int i,j;
	for (i = 0; i < MOD; i++)
		for (j = 0; j < LEN; j++)
			hash_process[i][j].alive = 0;
	printk(KERN_INFO "Done: clear all data from hash.\n");
	printk(KERN_INFO "Rand_yes = %d, Rand_no = %d.\n", rand_yes, rand_no);
	rand_yes = 0;
	rand_no = 0;
}

static void *rd_init_queue(struct request_queue *q)
{
	struct rd_data *nd;
	int i;
	nd = kmalloc_node(sizeof(*nd), GFP_KERNEL, q->node);
	if (!nd)
		return NULL;
	nd->q = q;
	for (i = 0; i <= MAXN_PRIO; i++)
		INIT_LIST_HEAD(&nd->prio_queue[i]);
	ndglobal = nd;
	nd->resume_timer.function = resume_timeout;
	nd->resume_timer.data = (unsigned long)q;
	init_timer(&nd->resume_timer);
	clear_hash();
	for (i = 0; i <= MAXN_PRIO; i++)
		cnt_processes[i] = 0;
	return nd;
}

static void rd_exit_queue(struct elevator_queue *e)
{
	struct rd_data *nd = e->elevator_data;
	int i;
	int flag = 1;
	del_timer_sync(&nd->resume_timer);
	for (i = 0; i <= MAXN_PRIO; i++)
		if(!list_empty(&nd->prio_queue[i]))
			flag = 0;
	BUG_ON(!flag);
	kfree(nd);
}

static struct elevator_type elevator_rd = {
	.ops = {
		.elevator_merge_req_fn		= rd_merged_requests,
		.elevator_dispatch_fn		= rd_dispatch,
		.elevator_add_req_fn		= rd_add_request,
		.elevator_queue_empty_fn	= rd_queue_empty,
		.elevator_former_req_fn		= rd_former_request,
		.elevator_latter_req_fn		= rd_latter_request,
		.elevator_init_fn		= rd_init_queue,
		.elevator_exit_fn		= rd_exit_queue,
	},
	.elevator_name = "nsrd",
	.elevator_owner = THIS_MODULE,
};

static void add_process(const char *s)
{
	int pid = -1, prio = -1;
	int num = 0, flag = 0;
	int key, i, find;
	while((*s) != 0) {
		if ((*s) >= '0' && (*s) <= '9') {
			num = num * 10 + ((*s) & 0x0f);
			flag = 1;
		}
		else if (flag == 1)
		{
			if (pid == -1)
				pid = num;
			else if (prio == -1)
				prio = num;
			flag = 0;
			num = 0;
		}
		s++;
	}
	if (pid == -1) {
		printk(KERN_INFO "Error: process id not found.\n");
		return;
	}
	if (prio == -1 || prio > MAXN_PRIO) prio = MAXN_PRIO;
	key = pid % MOD;
	find = 0;
	for (i = 0; i < LEN; i++) {
		if (pid == hash_process[key][i].pid) {
			hash_process[key][i].prio = prio;
			hash_process[key][i].alive = 1;
			find = 1;
			break;
		}
	}
	if (!find) {
		for (i = 0; i < LEN; i++) {
			if (!hash_process[key][i].alive) {
				hash_process[key][i].pid = pid;
				hash_process[key][i].prio = prio;
				hash_process[key][i].alive = 1;
				break;
			}
		}
	}
	printk(KERN_INFO "Done: add process (pid = %d, prio = %d) into hash.\n", pid, prio);
	return;
}

static void remove_process(const char *s)
{
	int pid = -1;
	int num = 0, flag = 0;
	int key, i, find;
	while((*s) != 0) {
		if ((*s) >= '0' && (*s) <= '9') {
			num = num * 10 + ((*s) & 0x0f);
			flag = 1;
		}
		else if (flag == 1)
		{
			if (pid == -1)
				pid = num;
			break;
		}
		s++;
	}
	if (pid == -1) {
		printk(KERN_INFO "Error: process id not found.\n");
		return;
	}
	key = pid % MOD;
	find = 0;
	for (i = 0; i < LEN; i++) {
		if (pid == hash_process[key][i].pid && hash_process[key][i].alive) {
			hash_process[key][i].alive = 0;
			find = 1;
			break;
		}
	}
	if (!find) {
		printk(KERN_INFO "Error: process (pid = %d) not found.\n", pid);
		return;
	}
	printk(KERN_INFO "Done: delete (pid = %d) from hash.\n", pid);
	return;
}

static void set_threshold(const char *s)
{
	int num = 0, flag = 0;
	while((*s) != 0) {
		if ((*s) >= '0' && (*s) <= '9') {
			num = num * 10 + ((*s) - '0');
			flag = 1;
		}
		else if (flag == 1)
		{
			flag = 0;
			break;
		}
		s++;
	}
	threshold = num;
	if (threshold > DEN) threshold = DEN;
}

static int rdnice_write( struct file *filp, const char __user *buff,
		unsigned long len, void *data )
{
	if(len > sizeof(proc_writebuf) - 1)
		return -EFBIG;
	if (copy_from_user(proc_writebuf, buff, len))
		return -EFAULT;
	proc_writebuf[len] = 0;
	if(proc_writebuf[0] == 'w')
	{
		printk(KERN_INFO "io waking up:j=%lu\n",jiffies);
		blk_run_queue(ndglobal->q);
		return len;
	}
	if (proc_writebuf[0] == 'a') add_process(proc_writebuf); // 格式：add pid prio，如不加参数prio则默认设置为最高优先级MAXN_PRIO，例：add 1234 3
	else if (proc_writebuf[0] == 'r') remove_process(proc_writebuf); // 格式：remove pid，例：remove 1234
	else if (proc_writebuf[0] == 'c') clear_hash(); // 格式：clear
	else if (proc_writebuf[0] == 't') set_threshold(proc_writebuf); // 格式：threshold num，num为0-10007(DEN)之间的整数，例：threshold 1000
	return len;
}

static int rdnice_read( char *page, char **start, off_t off,int count, int *eof, void *data )
{
	int len, i, j, pos, length;
	char oneline[100];
	if (off > 0) {
		*eof = 1;
		return 0;
	}
	pos = 0, length = 0;
	proc_readbuf[0] = 0;
	for (i = 0; i < MOD; i++)
		for (j = 0; j < LEN; j++)
			if (hash_process[i][j].alive == 1) {
				length = sprintf(oneline, "(pid = %d, prio = %d)\n", hash_process[i][j].pid, hash_process[i][j].prio);
				sprintf(proc_readbuf + pos, "%s", oneline);
				pos += length;
			}
	if (pos == 0) {
		length = sprintf(proc_readbuf + pos, "No data in hash\n");
		pos += length;
	}
	sprintf(proc_readbuf + pos, "Threshold: %d, Max: %d\n", threshold, DEN);
	len = sprintf(page, "%s", proc_readbuf);
	return len;
}

static int __init rd_init(void)
{
	int ret = 0;
	elv_register(&elevator_rd);
	//初始化proc文件
	proc_entry=create_proc_entry("rdnice",0644,NULL);
	if (proc_entry == NULL) {
		ret = -ENOMEM;
		printk(KERN_INFO "rd-iosched: Couldn't create proc entry\n");
	} else {
		proc_entry->read_proc = rdnice_read;
		proc_entry->write_proc = rdnice_write;
		//proc_entry->owner = THIS_MODULE;
		printk(KERN_INFO "rd-iosched: Module loaded.\n");
	}
	return ret;
}

static void __exit rd_exit(void)
{
	remove_proc_entry("rdnice",NULL);
	elv_unregister(&elevator_rd);
	printk(KERN_INFO "rd-iosched: Module removed.\n");
}

module_init(rd_init);
module_exit(rd_exit);

MODULE_AUTHOR("Team 17");
MODULE_LICENSE("GPL");
MODULE_DESCRIPTION("rd IO scheduler");
