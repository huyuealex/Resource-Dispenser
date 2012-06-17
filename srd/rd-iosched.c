/*
 * elevator rd
 */
#include <linux/blkdev.h>
#include <linux/elevator.h>
#include <linux/bio.h>
#include <linux/module.h>
#include <linux/init.h>
#include <linux/proc_fs.h>

static char procbuf[16];
static pid_t rtpid=0;//貌似有特殊进程pid为0,但是这里只要为0就当作没初始化
static const int rtnice=20;//表示多少次优先服务rtpid之后才服务一次其他进程
static int rtcount=0;
static int srprinttime=0,arprinttime=0,mrprinttime=0;
static struct proc_dir_entry *proc_entry;
static unsigned long dontuntil=0;

struct rd_data {
	struct list_head queue;
	struct list_head rtqueue;
};

static void rd_merged_requests(struct request_queue *q, struct request *rq,
				 struct request *next)
{
	list_del_init(&next->queuelist);
	//if(rtpid&&printtime++<16)
	//	printk(KERN_INFO "rd_merge_request pid:%i\n",current->pid);
	if(rtpid&&mrprinttime<16&&rq->elevator_private==(void *)1)
	{
		++mrprinttime;
		printk(KERN_INFO "rd_merged_request get it\n");
	}
}

static int rd_dispatch_force(struct request_queue *q)
{
	struct rd_data *nd = q->elevator->elevator_data;
	struct request *rq;
	if(!list_empty(&nd->rtqueue))
	{
		rq = list_entry(nd->rtqueue.next, struct request, queuelist);
		list_del_init(&rq->queuelist);
		//elv_dispatch_sort(q, rq);
		elv_dispatch_add_tail(q, rq);
		return 1;
	}
	if (!list_empty(&nd->queue)) {
		rq = list_entry(nd->queue.next, struct request, queuelist);
		list_del_init(&rq->queuelist);
		//elv_dispatch_sort(q, rq);
		elv_dispatch_add_tail(q, rq);
		return 1;
	}
	return 0;
}
static int rd_dispatch(struct request_queue *q, int force)
{
	struct rd_data *nd = q->elevator->elevator_data;
	struct request *rq;
	
	if(!list_empty(&nd->rtqueue))
	{
		rq = list_entry(nd->rtqueue.next, struct request, queuelist);
		list_del_init(&rq->queuelist);
		//elv_dispatch_sort(q, rq);
		elv_dispatch_add_tail(q, rq);
		//++rtcount;
		dontuntil=jiffies+HZ/200;//2ms后才允许普通任务进入队列
		return 1;
	}
	if ((force||time_after(jiffies,dontuntil))&&!list_empty(&nd->queue)) {
		rq = list_entry(nd->queue.next, struct request, queuelist);
		list_del_init(&rq->queuelist);
		//elv_dispatch_sort(q, rq);
		elv_dispatch_add_tail(q, rq);
		//rtcount=0;//每次服务完一次普通进程则重置rtcount
		return 1;
	}
	//if(!list_empty(&nd->rtqueue)&&rtcount<rtnice)
	//{
	//	rq = list_entry(nd->rtqueue.next, struct request, queuelist);
	//	list_del_init(&rq->queuelist);
	//	elv_dispatch_sort(q, rq);
	//	++rtcount;
	//	return 1;
	//}
	//if (!list_empty(&nd->queue)) {
	//	rq = list_entry(nd->queue.next, struct request, queuelist);
	//	list_del_init(&rq->queuelist);
	//	elv_dispatch_sort(q, rq);
	//	rtcount=0;//每次服务完一次普通进程则重置rtcount
	//	return 1;
	//}
	return 0;
}

static void rd_add_request(struct request_queue *q, struct request *rq)
{
	struct rd_data *nd = q->elevator->elevator_data;

	if(rq->elevator_private==(void *)1)
		list_add_tail(&rq->queuelist, &nd->rtqueue);
	else
		list_add_tail(&rq->queuelist, &nd->queue);
	//if(rtpid&&arprinttime<16&&rq->elevator_private==(void *)1)
	//{
	//	++arprinttime;
	//	printk(KERN_INFO "rd_add_request get it\n");
	//}
}

static int rd_queue_empty(struct request_queue *q)
{
	struct rd_data *nd = q->elevator->elevator_data;

	return list_empty(&nd->queue);
}

static struct request *
rd_former_request(struct request_queue *q, struct request *rq)
{
	struct rd_data *nd = q->elevator->elevator_data;

	if (rq->queuelist.prev == &nd->queue)
		return NULL;
	return list_entry(rq->queuelist.prev, struct request, queuelist);
}

static struct request *
rd_latter_request(struct request_queue *q, struct request *rq)
{
	struct rd_data *nd = q->elevator->elevator_data;

	if (rq->queuelist.next == &nd->queue)
		return NULL;
	return list_entry(rq->queuelist.next, struct request, queuelist);
}

static int rd_set_req(struct request_queue *q, struct request *rq, gfp_t gfp_mask)
{
	unsigned long flags;
	might_sleep_if(gfp_mask & __GFP_WAIT);//这句用处不明,但扔这大概没有什么坏处
	spin_lock_irqsave(q->queue_lock, flags);
	if(rtpid&&current->pid==rtpid)
	{
		if(srprinttime<16)
		{
			++srprinttime;
			printk(KERN_INFO "rd_set_req get it\n");
		}
		rq->elevator_private=(void *)1;
	}
	else
		rq->elevator_private=(void *)0;
	spin_unlock_irqrestore(q->queue_lock, flags);
	return 0;
}
static void *rd_init_queue(struct request_queue *q)
{
	struct rd_data *nd;

	nd = kmalloc_node(sizeof(*nd), GFP_KERNEL, q->node);
	if (!nd)
		return NULL;
	INIT_LIST_HEAD(&nd->queue);
	INIT_LIST_HEAD(&nd->rtqueue);
	return nd;
}

static void rd_exit_queue(struct elevator_queue *e)
{
	struct rd_data *nd = e->elevator_data;

	BUG_ON(!list_empty(&nd->queue));
	BUG_ON(!list_empty(&nd->rtqueue));
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

		.elevator_set_req_fn	=rd_set_req,
	},
	.elevator_name = "rd",
	.elevator_owner = THIS_MODULE,
};
static int rdatoi(const char *s)
{
	int result=0;
	while((*s)>='0'&&(*s)<='9')
	{
		result=result*10+((*s)&0x0f);
		++s;
	}
	if(*s!=0)//非法字符
		return 0;
	return result;
}
static int rdnice_write( struct file *filp, const char __user *buff,
	       	unsigned long len, void *data )
{
	if(len>sizeof(procbuf)-1)
		return -EFBIG;
	if (copy_from_user( procbuf, buff, len ))
		return -EFAULT;
	procbuf[len-1]=0;//无论用户传入串是否以0结尾,保证procbuf以0结尾
	rtpid=rdatoi(procbuf);
	arprinttime=0;
	mrprinttime=0;
	srprinttime=0;
	rtcount=0;
	printk(KERN_INFO "new rtpid=%i,jiffies=%lu\n",rtpid,jiffies);
	return len;
}
static int rdnice_read( char *page, char **start, off_t off,int count, int *eof, void *data )
{
	//测试用
	int len;
	//下面这个if不很清楚是干什么用的,貌似off>0表示目标缓冲区占据多页.那么就不处理直接退出
	if (off > 0) {
		*eof = 1;
		return 0;
	}
	len = sprintf(page, "%i\n", rtpid);
	return len;
}
static int __init rd_init(void)
{
	int ret=0;
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


MODULE_LICENSE("GPL");
MODULE_DESCRIPTION("rd IO scheduler");
