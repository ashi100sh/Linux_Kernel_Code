/*
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 */

//Reference example: http://www.programering.com/a/MDN4IjMwATk.html

#include <linux/kthread.h>
#include <linux/module.h>
#include <linux/slab.h>

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

static struct task_struct * MyThread = NULL;
static int MyPrintk(void *data)
{
        char *mydata = kmalloc(strlen(data)+1,GFP_KERNEL);
        memset(mydata,'\0',strlen(data)+1);
        strncpy(mydata,data,strlen(data));
        while(!kthread_should_stop())
        {
                SLEEP_MILLI_SEC(1000);
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
