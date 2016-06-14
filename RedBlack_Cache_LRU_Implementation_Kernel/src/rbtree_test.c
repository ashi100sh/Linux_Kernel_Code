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
#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/init.h>
#include "cache.h"

static int __init rb_test_init(void)
{
        struct fileInfo *filp = NULL;
        struct scan_cache_record *scan_cache_record_ptr = NULL;
        int ret = 0;
        printk("RB test init\n");
        init_scan_cache();

        filp = kzalloc(sizeof(struct fileInfo), GFP_KERNEL);
        if (filp) {
                filp->dev_id = 100;
                filp->inode_number = 1001;
                filp->inode_generation = 12345;
                ret = insert_scan_cache_record(filp);
                if (ret != 0)
                        printk("Fail to insert : %lu\n", filp->inode_number);

                kfree(filp);
                filp = NULL;
        }


        filp = kzalloc(sizeof(struct fileInfo), GFP_KERNEL);
        if (filp) {
                filp->dev_id = 100;
                filp->inode_number = 1009;
                filp->inode_generation = 12346;
                ret = insert_scan_cache_record(filp);
                if (ret != 0)
                        printk("Fail to insert : %lu\n", filp->inode_number);
                kfree(filp);
                filp = NULL;
        }


        filp = kzalloc(sizeof(struct fileInfo), GFP_KERNEL);
        if (filp) {
                filp->dev_id = 100;
                filp->inode_number = 1003;
                filp->inode_generation = 12347;
                ret = insert_scan_cache_record(filp);
                if (ret != 0)
                        printk("Fail to insert : %lu\n", filp->inode_number);

                kfree(filp);
                filp = NULL;
        }


        filp = kzalloc(sizeof(struct fileInfo), GFP_KERNEL);
        if (filp) {
                filp->dev_id = 100;
                filp->inode_number = 1002;
                filp->inode_generation = 12348;
                ret = insert_scan_cache_record(filp);
                if (ret != 0)
                        printk("Fail to insert : %lu\n", filp->inode_number);

                kfree(filp);
                filp = NULL;
        }


        filp = kzalloc(sizeof(struct fileInfo), GFP_KERNEL);
        if (filp) {
                filp->dev_id = 100;
                filp->inode_number = 1006;
                filp->inode_generation = 12349;
                ret = insert_scan_cache_record(filp);
                if (ret != 0)
                        printk("Fail to insert : %lu\n", filp->inode_number);

                kfree(filp);
                filp = NULL;
        }




        filp = kzalloc(sizeof(struct fileInfo), GFP_KERNEL);
        if (filp) {
                filp->dev_id = 99;
                filp->inode_number = 1006;
                filp->inode_generation = 12351;
                ret = insert_scan_cache_record(filp);
                if (ret != 0)
                        printk("Fail to insert : %lu\n", filp->inode_number);

                kfree(filp);
                filp = NULL;
        }


        filp = kzalloc(sizeof(struct fileInfo), GFP_KERNEL);
        if (filp) {
                filp->dev_id = 98;
                filp->inode_number = 1006;
                filp->inode_generation = 12352;
                ret = insert_scan_cache_record(filp);
                if (ret != 0)
                        printk("Fail to insert : %lu\n", filp->inode_number);

                kfree(filp);
                filp = NULL;
        }


        //Dupicate Entry , which should get updated as ID and inode_number key is same
        filp = kzalloc(sizeof(struct fileInfo), GFP_KERNEL);
        if (filp) {
                filp->dev_id = 100;
                filp->inode_number = 1006;
                filp->inode_generation = 12350;
                ret = insert_scan_cache_record(filp);
                if (ret != 0)
                        printk("Fail to insert : %lu\n", filp->inode_number);

                kfree(filp);
                filp = NULL;
        }

        //First Element:
        scan_cache_record_ptr = RB_FIRST(&scan_cache_rb_tree,
                                      struct scan_cache_record, cache_rb_node);
        if (scan_cache_record_ptr) {
                printk("First: %lu %lu %u\n", scan_cache_record_ptr->dev_id,
                       scan_cache_record_ptr->inode_number,
                       scan_cache_record_ptr->inode_generation);
        }


        //Last Element:
        scan_cache_record_ptr = RB_LAST(&scan_cache_rb_tree,
                                      struct scan_cache_record, cache_rb_node);
        if (scan_cache_record_ptr) {
                printk("Last: %lu %lu %u\n", scan_cache_record_ptr->dev_id,
                       scan_cache_record_ptr->inode_number,
                       scan_cache_record_ptr->inode_generation);
        }


        //Iterating
        scan_cache_record_ptr = RB_FIRST(&scan_cache_rb_tree,
                                      struct scan_cache_record, cache_rb_node);
        if (scan_cache_record_ptr) {
                printk("First: %lu %lu %u\n", scan_cache_record_ptr->dev_id,
                       scan_cache_record_ptr->inode_number,
                       scan_cache_record_ptr->inode_generation);
        }

        while (scan_cache_record_ptr) {
                scan_cache_record_ptr = RB_NEXT(scan_cache_record_ptr, cache_rb_node);
                if (scan_cache_record_ptr) {
                        printk("Next: %lu %lu %u\n", scan_cache_record_ptr->dev_id,
                        scan_cache_record_ptr->inode_number,
                        scan_cache_record_ptr->inode_generation);
                }
        }
        // End of iteration

        //Find and Delete
        filp = kzalloc(sizeof(struct fileInfo), GFP_KERNEL);
        if (filp) {
                bool ret;
                filp->dev_id = 98;
                filp->inode_number = 1006;
                filp->inode_generation = 12352;
                ret = find_remove_scan_cache_record(filp);
                if (ret == false)
                        printk("Fail to find and delete : %lu %lu %lu\n",
                               filp->dev_id,
                               filp->inode_number,
                               filp->inode_generation);
                kfree(filp);
                filp = NULL;
        }
#if 1
        printk("------ After Deleting Node: Iterating------------\n");
        //Iterating
        scan_cache_record_ptr = RB_FIRST(&scan_cache_rb_tree,
                                      struct scan_cache_record, cache_rb_node);
        if (scan_cache_record_ptr) {
                printk("First: %lu %lu %u\n", scan_cache_record_ptr->dev_id,
                       scan_cache_record_ptr->inode_number,
                       scan_cache_record_ptr->inode_generation);
        }

        while (scan_cache_record_ptr) {
                scan_cache_record_ptr = RB_NEXT(scan_cache_record_ptr, cache_rb_node);
                if (scan_cache_record_ptr) {
                        printk("Next: %lu %lu %u\n", scan_cache_record_ptr->dev_id,
                        scan_cache_record_ptr->inode_number,
                        scan_cache_record_ptr->inode_generation);
                }
        }
        // End of iteration
#endif


        return 0;    // Non-zero return means that the module couldn't be loaded.
}

static void __exit rb_test_cleanup(void)
{
        exit_scan_cache();
        printk(KERN_INFO "Cleaning up module.\n");
}

module_init(rb_test_init);
module_exit(rb_test_cleanup);

MODULE_LICENSE("GPL");
MODULE_AUTHOR("Ashish Singh");
MODULE_DESCRIPTION("RB Treee test programme");
