/* ************************************************************************
 *       Filename:  mmp_drv.c
 *    Description:  
 *        Version:  1.0
 *        Created:  2019年11月10日 02时29分17秒 PST
 *       Revision:  none
 *       Compiler:  gcc
 *         Author:  tiydy , 
 *        Company:  
 * ************************************************************************/

#include <linux/init.h>
#include <linux/module.h>
#include <linux/fs.h>
#include <linux/device.h>
#include <linux/interrupt.h>
#include <linux/gpio.h>
#include <linux/input.h>
#include <linux/wait.h>
#include <linux/sched.h>
#include <linux/poll.h>
#include <linux/slab.h>
#include <linux/fb.h>
#include <linux/mm.h>
#include <asm/uaccess.h> 
#include <asm/device.h> //下面这三个头文件是由于动态创建需要加的
#include <linux/device.h>
#include <linux/cdev.h>



void *virt_mem; //模拟的一块4k内存
unsigned long pos=0;

/* 定义幻数 */
#define MEMDEV_IOC_MAGIC  'k'
/* 定义命令 */
#define MEMDEV_IOCPRINT   _IO(MEMDEV_IOC_MAGIC, 1)
#define MEMDEV_IOCGETDATA _IOR(MEMDEV_IOC_MAGIC, 2, int)
#define MEMDEV_IOCSETDATA _IOW(MEMDEV_IOC_MAGIC, 3, int)
#define MEMDEV_IOC_MAXNR 3

#define DEVICENAME "ubuntu_lcd"
#define PAGE_SIZE (800*480*4)


struct cdev *cdev_t;
dev_t dev=0;
static struct class *cdev_class; 

static char bufrh[1024]="hello drv test!";

static int ubuntu_lcd_open(struct inode *inodep, struct file *filep)
{
    printk("read success!\n");
    return 0;
}

static int ubuntu_lcd_release(struct inode *inodep, struct file *filep)
{
    return 0;
}
static ssize_t ubuntu_lcd_read (struct file *filep, char __user *buf, size_t count, loff_t *offset)
{
    if(copy_to_user(buf, bufrh, strlen(bufrh)))
    {
        printk("copy_to_user fail!\n");
    }
    return 0;
}

static ssize_t ubuntu_lcd_write (struct file *filep, const char __user *buf,size_t count, loff_t *offse)
{
    printk("write!\n");
	unsigned char *ptr = (unsigned char *)virt_mem;
	if(copy_from_user(ptr+pos, buf, count))
	{
		printk("copy fail");
	}else{
		pos += count;
	}
	
    return 0;
}

static int ubuntu_lcd_mmap(struct file *filp, struct vm_area_struct *vma)
{

	unsigned long addr;
	addr = virt_to_phys(virt_mem);
 	if (io_remap_pfn_range(vma, vma->vm_start, addr >> PAGE_SHIFT,
					PAGE_SIZE, vma->vm_page_prot)) {
		printk(KERN_ERR "%s: io_remap_pfn_range failed\n",
			__func__);
		return -EAGAIN;
	}
  /*
#if 0
	unsigned long addr;
	addr = virt_to_phys(virt_mem);
	//将当前内存映射到用户空间
	//参数1--描述应用空间的mmap函数的需求(参数)
	//参数2--应用空间的起始位置
	//参数3--内核空间的内存的物理地址的页地址
	//参数4--内存的大小
	//参数5--权限
	if (remap_pfn_range(vma,vma->vm_start,vma->vm_pgoff,vma->vm_end -vma->vm_start,vma->vm_page_prot)) {
		printk(KERN_ERR "%s: io_remap_pfn_range failed\n",
			__func__);
		return -EAGAIN;
	}
	printk("mmap\n");
#else 
	int ret;
	//vma->vm_flags |= VM_RESERVED;
	//vma->vm_flags |= VM_IO;
	vma->vm_page_prot = pgprot_noncached(vma->vm_page_prot);
	// vma->vm_pgoff为用户层off, PAGE_SHIFT,即物理地址的页帧号,映射大小必为PAGE_SIZE整数倍 /
	ret =remap_pfn_range(vma,vma->vm_start,vma->vm_pgoff,vma->vm_end -vma->vm_start,vma->vm_page_prot);
	if(ret)
	{
		printk("remap_pfn_range err!\n");
		return -EAGAIN;
	}
	printk("In %s,pgoff = %lx, start= %lx,end = %lx\n",__func__,vma->vm_pgoff,vma->vm_start,vma->vm_end);
#endif
*/
	return 0;
}



static long ubuntu_lcd_ioctl(struct file *filp, unsigned int cmd, unsigned long arg)
{

    int err = 0;
    int ret = 0;
    int ioarg = 0;
    
    /* 检测命令的有效性 */
    if (_IOC_TYPE(cmd) != MEMDEV_IOC_MAGIC) 
        return -EINVAL;
    if (_IOC_NR(cmd) > MEMDEV_IOC_MAXNR) 
        return -EINVAL;

    /* 根据命令类型，检测参数空间是否可以访问 */
    if (_IOC_DIR(cmd) & _IOC_READ)
        err = !access_ok(VERIFY_WRITE, (void *)arg, _IOC_SIZE(cmd));
    else if (_IOC_DIR(cmd) & _IOC_WRITE)
        err = !access_ok(VERIFY_READ, (void *)arg, _IOC_SIZE(cmd));
    if (err) 
        return -EFAULT;

    /* 根据命令，执行相应的操作 */
    switch(cmd) {

      /* 打印当前设备信息 */
      case MEMDEV_IOCPRINT:
      	printk("<--- CMD MEMDEV_IOCPRINT Done--->\n\n");
        break;
      
      /* 获取参数 */
      case MEMDEV_IOCGETDATA: 
        ioarg = 1101;
        ret = __put_user(ioarg, (int *)arg);
        break;
      
      /* 设置参数 */
      case MEMDEV_IOCSETDATA: 
        ret = __get_user(ioarg, (int *)arg);
        printk("<--- In Kernel MEMDEV_IOCSETDATA ioarg = %d --->\n\n",ioarg);
        break;

      default:  
        return -EINVAL;
    }
    return ret;

}

static const struct file_operations fops = {
    .owner = THIS_MODULE,
    .open = ubuntu_lcd_open,
    .release = ubuntu_lcd_release,
    .read = ubuntu_lcd_read,
    .write = ubuntu_lcd_write,
	.mmap=ubuntu_lcd_mmap,
	.unlocked_ioctl=ubuntu_lcd_ioctl,

};

static int __init ubuntu_lcd_init(void)
{
    int ret = alloc_chrdev_region(&dev, 0, 1, DEVICENAME);
    if(ret)
    {
    	printk("mydrv register fail");
		unregister_chrdev_region(dev,1);
	return ret;
    }else
    {
    	cdev_t =cdev_alloc();
    	cdev_t->owner=THIS_MODULE;
    	cdev_init(cdev_t, &fops);
    	ret = cdev_add(cdev_t, dev, 1);
		if(ret)
		{
			printk("cdev_add fail\n");
			unregister_chrdev_region(dev,1);
			return ret;
		}
		//动态创建设备节点
		cdev_class = class_create(THIS_MODULE,DEVICENAME);
		if(IS_ERR(cdev_class))
		{ 
			printk("ERR:cannot create a cdev_class\n");
			unregister_chrdev_region(dev,1);
			return -1;
		}
		device_create(cdev_class,NULL, dev, 0, DEVICENAME);
		virt_mem = kzalloc(PAGE_SIZE,  GFP_KERNEL);
		printk("init\n");
	}
    return 0;
}

static void __exit ubuntu_lcd_cleanup(void)
{
	kfree(virt_mem);
    cdev_del(cdev_t);
    device_destroy(cdev_class, dev);
	class_destroy(cdev_class);
	unregister_chrdev_region(dev,1);
	
}

module_init(ubuntu_lcd_init);
module_exit(ubuntu_lcd_cleanup);
MODULE_LICENSE("GPL");

