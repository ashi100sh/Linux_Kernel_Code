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
 *
 */
/*Source: http://fxr.watson.org/fxr/source/samples/kfifo/record-example.c?v=linux-2.6 */

//RHEL-6.x support
#include <linux/kernel.h>
#include <linux/init.h>
#include <linux/module.h>
#include <linux/proc_fs.h>
#include <linux/mutex.h>
#include <linux/kfifo-new.h>

/* fifo size in elements (bytes) */
#define FIFO_SIZE       10*8

struct kfifo_rec_ptr_1 test;

static int __init testfunc(void)
{
        char    buf[100];
        unsigned int i;
        unsigned int ret;

        /* put in variable length data */
        for (i = 0; i < 10; i++)
        {
                memset(buf, 'a' + i, i + 1);
                kfifo_in(&test, buf, i + 1);
        }


        printk(KERN_INFO "fifo len: %u\n", kfifo_len(&test));
        /* show the first record without removing from the fifo */
        ret = kfifo_out_peek(&test, buf, sizeof(buf));
        buf[ret] = '\0';
        if (ret)
                printk(KERN_INFO "First Record = %.*s\n\n", ret, buf);

        /* check the correctness of all values in the fifo */
        while (!kfifo_is_empty(&test))
        {
                ret = kfifo_out(&test, buf, sizeof(buf));
                buf[ret] = '\0';
                printk(KERN_INFO "item = %.*s\n", ret, buf);
        }

        return 0;
}


static int __init example_init(void)
{
        int ret;
        ret = kfifo_alloc(&test, FIFO_SIZE, GFP_KERNEL);
        if (ret)
        {
                printk(KERN_ERR "error kfifo_alloc\n");
                return ret;
        }

        if (testfunc() < 0)
        {
                kfifo_free(&test);
                return -EIO;
        }

        return 0;
}


static void __exit example_exit(void)
{
        kfifo_free(&test);
}

module_init(example_init);
module_exit(example_exit);
MODULE_LICENSE("GPL");
