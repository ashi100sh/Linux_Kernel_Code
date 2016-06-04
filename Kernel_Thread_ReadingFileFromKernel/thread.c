#include <linux/kthread.h>
#include <linux/module.h>
#include <linux/slab.h>
#include <linux/fs.h>      // Needed by filp
#include <asm/uaccess.h>   // Needed by segment descriptors
#include <linux/delay.h>

#ifndef SLEEP_MILLI_SEC
#define SLEEP_MILLI_SEC(nMilliSec)\
        do { \
                long timeout = (nMilliSec) * HZ / 1000; \
                while(timeout > 0) \
                { \
                        timeout = schedule_timeout(timeout); \
                } \
        }while(0);
#endif

void ReadFile()
{

        struct file *f;
        char buf[128];
        mm_segment_t fs;
        int i;
        // Init the buffer with 0
        for(i=0;i<128;i++)
                buf[i] = 0;
        // To see in /var/log/messages that the module is operating
        printk(KERN_INFO "My module is loaded\n");
        // I am using Fedora and for the test I have chosen following file
        // Obviously it is much smaller than the 128 bytes, but hell with it =)
        // Create the file "/home/fedora-release" and write some content 
        f = filp_open("/home/fedora-release", O_RDONLY, 0);
        if(f == NULL)
                printk(KERN_ALERT "filp_open error!!.\n");
        else{
                // Get current segment descriptor
                fs = get_fs();
                // Set segment descriptor associated to kernel space
                set_fs(get_ds());
                // Read the file
                f->f_op->read(f, buf, 128, &f->f_pos);
                // Restore segment descriptor
                // Restore segment descriptor
                set_fs(fs);
                // See what we read from file
                printk(KERN_INFO "buf:%s\n",buf);
        }
        filp_close(f,NULL);
}

static struct task_struct * MyThread = NULL;
static int MyPrintk(void *data)
{
        char *mydata = kmalloc(strlen(data)+1,GFP_KERNEL);
        memset(mydata,'\0',strlen(data)+1);
        strncpy(mydata,data,strlen(data));
        ReadFile();
        while(!kthread_should_stop())
        {
                SLEEP_MILLI_SEC(1000);
                ReadFile();
                printk("%s\n",mydata);
        }
        kfree(mydata);
        return 0;
}


static int __init init_kthread(void)
{
        MyThread = kthread_run(MyPrintk,"hello world","mythread");
        return 0;
}

static void __exit exit_kthread(void)
{
        if(MyThread)
        {
                printk("stop MyThread\n");
                kthread_stop(MyThread);
        }
}

module_init(init_kthread);
module_exit(exit_kthread);
