/*
 * elevator rd
 */
#include <linux/blkdev.h>
#include <linux/elevator.h>
#include <linux/bio.h>
#include <linux/module.h>
#include <linux/init.h>
#include <linux/proc_fs.h>

static char proctest[16];
static struct proc_dir_entry *proc_entry;

struct rd_data {
	struct list_head queue;
};

static void rd_merged_requests(struct request_queue *q, struct request *rq,
				 struct request *next)
{
	list_del_init(&next->queuelist);
}

static int rd_dispatch(struct request_queue *q, int force)
{
	struct rd_data *nd = q->elevator->elevator_data;

	if (!list_empty(&nd->queue)) {
		struct request *rq;
		rq = list_entry(nd->queue.next, struct request, queuelist);
		list_del_init(&rq->queuelist);
		elv_dispatch_sort(q, rq);
		return 1;
	}
	return 0;
}

static void rd_add_request(struct request_queue *q, struct request *rq)
{
	struct rd_data *nd = q->elevator->elevator_data;

	list_add_tail(&rq->queuelist, &nd->queue);
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

static void *rd_init_queue(struct request_queue *q)
{
	struct rd_data *nd;

	nd = kmalloc_node(sizeof(*nd), GFP_KERNEL, q->node);
	if (!nd)
		return NULL;
	INIT_LIST_HEAD(&nd->queue);
	return nd;
}

static void rd_exit_queue(struct elevator_queue *e)
{
	struct rd_data *nd = e->elevator_data;

	BUG_ON(!list_empty(&nd->queue));
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
	.elevator_name = "rd",
	.elevator_owner = THIS_MODULE,
};
static int rdnice_write( struct file *filp, const char __user *buff,
	       	unsigned long len, void *data )
{
	//测试用
	unsigned long len2=len;
	if(len2>sizeof(proctest))
		len2=sizeof(proctest);
	if (copy_from_user( proctest, buff, len2 ))
		return -EFAULT;
	proctest[len2-1]=0;
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
	len = sprintf(page, "%s\n", proctest);
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
