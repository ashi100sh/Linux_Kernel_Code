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

#include <linux/kernel.h>
#include <linux/init.h>
#include <linux/module.h>
#include <linux/interrupt.h>
#include <linux/wait.h>
#include <linux/delay.h>
#include <linux/kthread.h>
#include "queue.h"

static int status1 = 1;
static int status2 = 1;

struct scanRecord {
        TAILQ_ENTRY(scanRecord) link;
        int datum;
};

TAILQ_HEAD(scanQueueHead, scanRecord);

static struct scanQueueHead scanQueue = TAILQ_HEAD_INITIALIZER(scanQueue);
static spinlock_t scanQueueLock = SPIN_LOCK_UNLOCKED;


static DECLARE_WAIT_QUEUE_HEAD(wait_thread1);
static DECLARE_WAIT_QUEUE_HEAD(wait_thread2);

struct task_struct *thread1;
struct task_struct *thread2;

static int test_thread1(void *data)
{
        int ret;
        int count = 0;
        struct scanRecord *tempInsert;

        printk(KERN_INFO "Inside: %s\n", __FUNCTION__);

        /* Request delivery of SIGKILL */
        allow_signal(SIGKILL);
        allow_signal(SIGTERM);

        while (1)
        {
                /* Timeout is 2 second */
                ret = wait_event_interruptible_timeout(wait_thread1,
                                                        status1 == 0, HZ * 2);

                /* Die if I receive SIGKILL */
                if (signal_pending(current))
                {
                        printk(KERN_INFO "SIGKILL signal received for"
                               "thread1\n");
                        break;
                }

                printk("Thread1 : count:%d\n", count);

                tempInsert = kmalloc(sizeof(struct scanRecord), GFP_KERNEL);
                if (tempInsert)
                {
                        tempInsert->datum = count;
                        spin_lock(&scanQueueLock);
                        TAILQ_INSERT_TAIL(&scanQueue, tempInsert, link);
                        spin_unlock(&scanQueueLock);
                }

                count++;

                if (kthread_should_stop())
                {
                        break;
                }

        }


        printk(KERN_INFO "End of Thread1\n");
        return 0;
}

static int test_thread2(void *data)
{
        int ret;
        int count = 0;
        struct scanRecord *tempInsert;
        struct scanRecord *tmp;

        printk(KERN_INFO "Inside: %s\n", __FUNCTION__);

        allow_signal(SIGKILL);
        allow_signal(SIGTERM);

        while (1)
        {
                ret = wait_event_interruptible_timeout(wait_thread2,
                                                status2 == 0, HZ * 2);

                /* Die if I receive SIGKILL */
                if (signal_pending(current))
                {
                        printk(KERN_INFO "SIGKILL signal received for"
                               "thread2\n");
                        break;
                }

                printk("Thread2 : count:%d\n", count);
                count++;


                spin_lock(&scanQueueLock);
                TAILQ_FOREACH_SAFE(tempInsert, tmp, &scanQueue, link)
                {
                        printk("Removing %d\n", tempInsert->datum);
                        TAILQ_REMOVE(&scanQueue, tempInsert, link);
                        if (tempInsert)
                        {
                                kfree(tempInsert);
                        }
                }
                spin_unlock(&scanQueueLock);

                if (kthread_should_stop())
                {
                        break;
                }

        }

        printk(KERN_INFO "End of test thread2\n");
        return 0;
}



static int __init test_init(void)
{
        printk(KERN_INFO "Start: %s\n", __FUNCTION__);

        /* start the thread1 */
        status1 = 1;
        status2 = 1;
        thread1 = kthread_run(&test_thread1, NULL, "test_thread1");

        /* start the thread2 */
        thread2 = kthread_run(&test_thread2, NULL, "test_thread2");

        return 0;
}

static void __exit test_exit(void)
{
        printk(KERN_INFO "End: %s\n", __FUNCTION__);
        status1 = 0;
        wake_up(&wait_thread1);
        kthread_stop(thread1);

        status2 = 0;
        wake_up(&wait_thread2);
        kthread_stop(thread2);
}

module_init(test_init);
module_exit(test_exit);

MODULE_LICENSE("GPL");
MODULE_AUTHOR("Ashish");
