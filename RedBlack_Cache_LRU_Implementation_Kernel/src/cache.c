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
#include <linux/slab.h>
#include <linux/version.h>
#include "cache.h"

struct kmem_cache *ptr_scan_cache_slab __read_mostly;
struct kmem_cache *ptr_lru_cache_slab __read_mostly;
struct rb_root scan_cache_rb_tree = RB_ROOT;
struct rb_root lru_cache_rb_tree = RB_ROOT;
#if (LINUX_VERSION_CODE >= KERNEL_VERSION(2,6,39))
spinlock_t scan_cache_lock = __SPIN_LOCK_UNLOCKED(scan_cache_lock);
spinlock_t lru_cache_lock = __SPIN_LOCK_UNLOCKED(lru_cache_lock);
#else
spinlock_t scan_cache_lock = SPIN_LOCK_UNLOCKED;
spinlock_t lru_cache_lock = SPIN_LOCK_UNLOCKED;
#endif

int insert_lru_cache_record(struct timespec time_stamp, dev_t dev_id,
                         unsigned long inode_number, __u32 inode_generation);
static void remove_lru_cache_record(struct lru_cache_record *lru_cache_record_ptr);
static void remove_scan_cache_record(
                                struct scan_cache_record *scan_cache_record_ptr);
static struct timespec get_time_stamp(void)
{
        struct timespec current_time;
        getnstimeofday(&current_time);
        return current_time;
}

static inline int scan_cache_insert_compare_table(struct scan_cache_record *first_ptr,
                                          struct scan_cache_record *second_ptr)
{
        int result = 0;
        int64_t difference;

        /* If either of NULL return 0 */
        if (!first_ptr || !second_ptr)
                goto out;

        do {
                /* dev_id is first level of comparsion */
                difference = first_ptr->dev_id - second_ptr->dev_id;
                if (difference > 0) {
                        result = 1;
                        break;
                } else if (difference < 0) {
                        result = -1;
                        break;
                }

                /* inode_number is second level of comparsion */
                difference = first_ptr->inode_number - second_ptr->inode_number;
                if (difference > 0) {
                        result = 1;
                        break;
                } else if (difference < 0) {
                        result = -1;
                        break;
                }

        } while (0);

out:
        return result;
}


static inline int scan_cache_find_compare_table(struct scan_cache_record_compare *scan_cache_record_compare_ptr,
                                struct scan_cache_record *scan_cache_record_ptr)
{
        int result = 0;
        int64_t difference;

        /* If either of NULL return 0 */
        if (!scan_cache_record_ptr || !scan_cache_record_compare_ptr)
                goto out;

#if 0
        printk("scan_cache_record_ptr FindCompareTable: %lu %lu %lu\n",
               scan_cache_record_ptr->dev_id,
                scan_cache_record_ptr->inode_number,
                scan_cache_record_ptr->inode_generation);

        printk("scan_cache_record_compare_ptr FindCompareTable: %lu %lu %lu\n",
               scan_cache_record_compare_ptr->dev_id,
                scan_cache_record_compare_ptr->inode_number,
                scan_cache_record_compare_ptr->inode_generation);
#endif
        do {
                /* dev_id is first level of comparsion */
                difference = scan_cache_record_compare_ptr->dev_id - scan_cache_record_ptr->dev_id;
                if (difference > 0) {
                        result = 1;
                        break;
                } else if (difference < 0) {
                        result = -1;
                        break;
                }

                /* inode_number is second level of comparsion */
                difference = scan_cache_record_compare_ptr->inode_number - scan_cache_record_ptr->inode_number;
                if (difference > 0) {
                        result = 1;
                        break;
                } else if (difference < 0) {
                        result = -1;
                        break;
                }

                /* inode_generation is thrid level of comparsion */
                difference = scan_cache_record_compare_ptr->inode_generation - scan_cache_record_ptr->inode_generation;
                if (difference > 0) {
                        result = 1;
                        break;
                } else if (difference < 0) {
                        result = -1;
                        break;
                }

                /* If it reach here means entry found it */

        } while (0);

out:
        return result;
}

static inline int lru_cache_insert_compare_table(struct lru_cache_record *first_ptr,
                                         struct lru_cache_record *second_ptr)
{
        int result = 0;
        int64_t difference;

#if 0
        printk("First lru: time_stamp:%lld %lld %lu %lu\n",
               first_ptr->time_stamp.tv_sec,
               first_ptr->time_stamp.tv_nsec,
               first_ptr->dev_id,
               first_ptr->inode_number);

        printk("Second lru: time_stamp:%lld %lld %lu %lu\n",
               second_ptr->time_stamp.tv_sec,
               second_ptr->time_stamp.tv_nsec,
               second_ptr->dev_id,
               second_ptr->inode_number);
#endif

        do {
                difference = first_ptr->time_stamp.tv_sec - second_ptr->time_stamp.tv_sec;
                if (difference > 0) {
                        result = 1;
                        break;
                } else if (difference < 0) {
                        result = -1;
                        break;
                }

                difference = ((first_ptr->time_stamp.tv_nsec)) -
                       ((second_ptr->time_stamp.tv_nsec));

                if (difference > 0) {
                        result = 1;
                        break;
                } else if (difference < 0) {
                        result = -1;
                        break;
                }

        } while(0);

        return result;
}


static inline int lru_cache_find_compare_table(struct lru_cache_record_compare *first_ptr,
                                         struct lru_cache_record *second_ptr)
{
        int result = 0;
        int64_t difference;

#if 0
        printk("First lru: time_stamp:%lld %lld %lu %lu\n",
               first_ptr->time_stamp.tv_sec,
               first_ptr->time_stamp.tv_nsec,
               first_ptr->dev_id,
               first_ptr->inode_number);

        printk("Second lru: time_stamp:%lld %lld %lu %lu\n",
               second_ptr->time_stamp.tv_sec,
               second_ptr->time_stamp.tv_nsec,
               second_ptr->dev_id,
               second_ptr->inode_number);
#endif

        do {
                difference = first_ptr->time_stamp.tv_sec - second_ptr->time_stamp.tv_sec;
                if (difference > 0) {
                        result = 1;
                        break;
                } else if (difference < 0) {
                        result = -1;
                        break;
                }

                /* Convert : nsec to usec and then difference */
                difference = ((first_ptr->time_stamp.tv_nsec)) -
                       ((second_ptr->time_stamp.tv_nsec));

                if (difference > 0) {
                        result = 1;
                        break;
                } else if (difference < 0) {
                        result = -1;
                        break;
                }

        } while(0);

        return result;
}
#if 0
/* https://www.kernel.org/doc/Documentation/rbtree.txt */
static int insert_in_rb_tree(struct rb_root *root,
                   struct scan_cache_record *scan_cache_record_ptr) {

        struct rb_node **new = &(root->rb_node), *parent = NULL;
        int result, ret = -1;
        /* Figure out where to put new node */
        while (*new) {
                struct scan_cache_record *this = NULL;
                parent = *new;
                this = rb_entry(*new, struct scan_cache_record, cache_rb_node);
                result = scan_cache_insert_compare_table(scan_cache_record_ptr, this);
                //printk("Value of result:%d\n", result);
                if (result < 0)
                        new = &((*new)->rb_left);
                else if (result > 0)
                        new = &((*new)->rb_right);
                else
                        goto out;  /* It is already present */
        }

        /* Add new node and rebalance tree. */
        rb_link_node(&scan_cache_record_ptr->cache_rb_node, parent, new);
        rb_insert_color(&scan_cache_record_ptr->cache_rb_node, root);
        ret = 0;
out:
        return ret;

}

static void remove_in_rb_tree(struct rb_root *root,
                    struct scan_cache_record *scan_cache_record_ptr)
{
        rb_erase(&scan_cache_record_ptr->cache_rb_node, root);
}

static struct scan_cache_record * find_in_rb_tree(struct rb_root *root,
                                      struct scan_cache_record_compare *keyPtr)
{
        struct rb_node *rb_search_node = root->rb_node;
        printk("Inside %s\n", __FUNCTION__);
        printk("Value keyPtr: %lu %lu %lu \n", keyPtr->dev_id, keyPtr->inode_number,
                                                keyPtr->inode_generation);
        while (rb_search_node) {
                int result;
                struct scan_cache_record *search_data = rb_entry(rb_search_node,
                                        struct scan_cache_record, cache_rb_node);

                result = scan_cache_find_compare_table(keyPtr, search_data);
                printk("Value of result:%d\n", result);
                if (result < 0)
                        rb_search_node = rb_search_node->rb_left;
                else if (result > 0)
                        rb_search_node = rb_search_node->rb_right;
                else
                        return search_data;
        }

        return NULL;
}

int find_update_in_tree(struct rb_root *root,
                     struct scan_cache_record *new_scan_cache_record_ptr)
{
        int ret = -1;
        struct rb_node *rb_search_node = root->rb_node;
        while (rb_search_node) {
                int result;
                struct scan_cache_record *search_data = rb_entry(rb_search_node,
                                        struct scan_cache_record, cache_rb_node);

                // Here insertcompare table use to search on basic of devid and
                // inode, in newScan cache have new inode generation number
                result = scan_cache_insert_compare_table(new_scan_cache_record_ptr,
                                                     search_data);

                if (result < 0) {
                        rb_search_node = rb_search_node->rb_left;
                } else if (result > 0) {
                        rb_search_node = rb_search_node->rb_right;
                } else {
                        // Data found and now replace it
                        rb_replace_node(&search_data->cache_rb_node,
                                &new_scan_cache_record_ptr->cache_rb_node, root);
                        kmem_cache_free(ptr_scan_cache_slab, search_data);
                        ret = 0;
                        break;
                }
        }
        return ret;
}

#endif



static struct scan_cache_record * scan_cache_alloc(void)
{
        struct scan_cache_record *scan_cache_record_ptr = NULL;
        scan_cache_record_ptr = kmem_cache_alloc(ptr_scan_cache_slab, GFP_KERNEL);
        if (!scan_cache_record_ptr)
                printk("Fail to allocate memory for scan_cache_record\n");

        return scan_cache_record_ptr;
}

static struct lru_cache_record *lru_cache_alloc(void)
{
        struct lru_cache_record *lru_cache_record_ptr = NULL;
        lru_cache_record_ptr = kmem_cache_alloc(ptr_lru_cache_slab, GFP_KERNEL);
        if (!lru_cache_record_ptr)
                printk("Fail to allocate memory for lruCacheRecord\n");

        return lru_cache_record_ptr;
}

int find_and_update_lru_cache_record(struct scan_cache_record *scan_cache_record_old_ptr,
                                struct scan_cache_record *scan_cache_record_new_ptr)
{
        int ret = -1;
        struct lru_cache_record *lru_cache_record_search_ptr = NULL;
        struct lru_cache_record_compare lru_cache_key;
        lru_cache_key.time_stamp = scan_cache_record_old_ptr->time_stamp;
        spin_lock(&lru_cache_lock);
        RB_SEARCH(&lru_cache_rb_tree, lru_cache_record_search_ptr,
                  &lru_cache_key, lru_rb_node,
                  lru_cache_find_compare_table);
        spin_unlock(&lru_cache_lock);

        if (lru_cache_record_search_ptr) {
                remove_lru_cache_record(lru_cache_record_search_ptr);
                ret = insert_lru_cache_record(scan_cache_record_new_ptr->time_stamp,
                                        scan_cache_record_new_ptr->dev_id,
                                        scan_cache_record_new_ptr->inode_number,
                                        scan_cache_record_new_ptr->inode_generation);

                if (ret != 0) {
                        printk("ERROR: Fail to insert updated"
                               "lru record\n");
                }
        }
        return ret;
}

int find_and_update_scan_cache_record(
                                struct scan_cache_record *scan_cache_record_new_ptr)
{
        int ret = -1;
        struct scan_cache_record *scan_cache_record_search_ptr = NULL;
        struct scan_cache_record *scan_cache_record_old_ptr = NULL;
        if (scan_cache_record_new_ptr) {
                spin_lock(&scan_cache_lock);
                RB_SEARCH(&scan_cache_rb_tree, scan_cache_record_search_ptr,
                                scan_cache_record_new_ptr, cache_rb_node,
                                scan_cache_insert_compare_table);
                spin_unlock(&scan_cache_lock);

                if (scan_cache_record_search_ptr) {
                        scan_cache_record_old_ptr = scan_cache_record_search_ptr;
                        spin_lock(&scan_cache_lock);
                        RB_REPLACE(&scan_cache_rb_tree, scan_cache_record_old_ptr,
                                   scan_cache_record_new_ptr, cache_rb_node);
                        spin_unlock(&scan_cache_lock);

                        ret =find_and_update_lru_cache_record(scan_cache_record_old_ptr,
                                                    scan_cache_record_new_ptr);
                        if (ret != 0) {
                                remove_scan_cache_record(scan_cache_record_new_ptr);
                        }
                        // Delete Old scan_cache_record memory since it get
                        // replace, otherwise it cause memory leak
                        kmem_cache_free(ptr_scan_cache_slab, scan_cache_record_old_ptr);
                } else {
                        // Fail to fail old entry, so not able to remove
                        // therefore remove mem
                        kmem_cache_free(ptr_scan_cache_slab, scan_cache_record_new_ptr);
                        printk("Fail to search old record \n");
                }
        }

        return ret;
}


int insert_lru_cache_record(struct timespec time_stamp, dev_t dev_id,
                         unsigned long inode_number, __u32 inode_generation)
{
        struct lru_cache_record *lru_cache_record_ptr = NULL;
        int ret = -1;

        lru_cache_record_ptr = lru_cache_alloc();
        if (lru_cache_record_ptr) {
                lru_cache_record_ptr->time_stamp = time_stamp;
                lru_cache_record_ptr->dev_id = dev_id;
                lru_cache_record_ptr->inode_number = inode_number;
                lru_cache_record_ptr->inode_generation = inode_generation;

#if 1
                printk("Value of LRU: Timestamp:%lld %lld %lu %lu\n",
                       lru_cache_record_ptr->time_stamp.tv_sec,
                       lru_cache_record_ptr->time_stamp.tv_nsec,
                       lru_cache_record_ptr->dev_id,
                       lru_cache_record_ptr->inode_number);
#endif
                spin_lock(&lru_cache_lock);
                ret = RB_INSERT(&lru_cache_rb_tree, lru_cache_record_ptr, lru_rb_node,
                          lru_cache_insert_compare_table);
                spin_unlock(&lru_cache_lock);
        }

        return ret;
}

int insert_scan_cache_record(struct fileInfo *filp)
{
        struct scan_cache_record *scan_cache_record_ptr = NULL;
        int ret = 0;
        scan_cache_record_ptr = scan_cache_alloc();
        if (!scan_cache_record_ptr) {
                printk("Fail to allocate scan_cache_record_ptr in %s\n",
                                                        __FUNCTION__);
                ret = -ENOMEM;
                goto out;
        }

        scan_cache_record_ptr->dev_id = filp->dev_id;
        scan_cache_record_ptr->inode_number = filp->inode_number;
        scan_cache_record_ptr->inode_generation = filp->inode_generation;
        scan_cache_record_ptr->time_stamp = get_time_stamp();

        spin_lock(&scan_cache_lock);
        ret = RB_INSERT(&scan_cache_rb_tree, scan_cache_record_ptr, cache_rb_node,
                        scan_cache_insert_compare_table);
        spin_unlock(&scan_cache_lock);

        if (ret != 0) {
                if (ret == -1)
                        printk("Alredy present: %lu %lu %lu\n",
                               scan_cache_record_ptr->dev_id,
                               scan_cache_record_ptr->inode_number,
                               scan_cache_record_ptr->inode_generation);
                        //It already present in cache, it rare that same entry
                        //coming again, it means inode_generation is differenceernt, 
                        //in that case use rb_replace_node() which requrie find old
                        //entry and replace with newly called insert
                        //scan_cache_record.
                        ret = find_and_update_scan_cache_record(scan_cache_record_ptr);
                        //if it fail it delete the scan_cache_record_ptr so no need
                        //to cleanup
        } else {
                /* Insert cache successfully, now insert in LRU */
                ret = insert_lru_cache_record(scan_cache_record_ptr->time_stamp,
                                     scan_cache_record_ptr->dev_id,
                                     scan_cache_record_ptr->inode_number,
                                     scan_cache_record_ptr->inode_generation);
                if (ret != 0) {
                        remove_scan_cache_record(scan_cache_record_ptr);
                }
        }
out:
        return ret;
}

static struct scan_cache_record * find_scan_cache_record(
                                struct scan_cache_record_compare *keyPtr)
{
        struct scan_cache_record *scan_cache_record_ptr = NULL;
        printk("Inside %s\n", __FUNCTION__);
        if (keyPtr) {
                spin_lock(&scan_cache_lock);
                RB_SEARCH(&scan_cache_rb_tree, scan_cache_record_ptr, keyPtr,
                          cache_rb_node, scan_cache_find_compare_table);
                spin_unlock(&scan_cache_lock);
        }

        if (!scan_cache_record_ptr)
                printk("Fail to find in CacheRecord\n");
        return scan_cache_record_ptr;
}

static void remove_lru_cache_record(struct lru_cache_record *lru_cache_record_ptr)
{
        if (lru_cache_record_ptr) {
                spin_lock(&lru_cache_lock);
                RB_REMOVE(&lru_cache_rb_tree, lru_cache_record_ptr, lru_rb_node);
                spin_unlock(&lru_cache_lock);
                kmem_cache_free(ptr_lru_cache_slab, lru_cache_record_ptr);
        }
}

static void remove_scan_cache_record(
                                struct scan_cache_record *scan_cache_record_ptr)
{
        if (scan_cache_record_ptr) {
                spin_lock(&scan_cache_lock);
                RB_REMOVE(&scan_cache_rb_tree, scan_cache_record_ptr, cache_rb_node);
                spin_unlock(&scan_cache_lock);
                kmem_cache_free(ptr_scan_cache_slab, scan_cache_record_ptr);
        }
}

struct lru_cache_record *find_lru_cache_record(
                        struct lru_cache_record_compare *lru_cache_key_ptr)
{
        struct lru_cache_record *lru_cache_record_ptr = NULL;
        if (lru_cache_key_ptr) {
                spin_lock(&lru_cache_lock);
                RB_SEARCH(&lru_cache_rb_tree, lru_cache_record_ptr, lru_cache_key_ptr,
                                        lru_rb_node, lru_cache_find_compare_table);
                spin_unlock(&lru_cache_lock);
        }

        if (!lru_cache_record_ptr)
                printk("Error: Fail to find LRU record\n");

        return lru_cache_record_ptr;
}

bool find_remove_lru_cache_record(struct timespec time_stamp)
{
        bool ret = false;
        struct lru_cache_record *lru_cache_record_ptr;
        struct lru_cache_record_compare lru_cache_key_ptr;
        lru_cache_key_ptr.time_stamp = time_stamp;

        lru_cache_record_ptr = find_lru_cache_record(&lru_cache_key_ptr);
        if (lru_cache_record_ptr) {
                remove_lru_cache_record(lru_cache_record_ptr);
                ret = true;
        }

        return ret;
}

bool find_remove_scan_cache_record(struct fileInfo *filp)
{
        struct scan_cache_record *scan_cache_record_ptr = NULL;
        struct scan_cache_record_compare scan_cache_key_ptr;
        printk("Inside %s\n", __FUNCTION__);
        scan_cache_key_ptr.dev_id = filp->dev_id;
        scan_cache_key_ptr.inode_number = filp->inode_number;
        scan_cache_key_ptr.inode_generation = filp->inode_generation;
        scan_cache_record_ptr = find_scan_cache_record(&scan_cache_key_ptr);
#if 1
        if (scan_cache_record_ptr)
                printk("Search : %lu %lu %u\n", scan_cache_record_ptr->dev_id,
                        scan_cache_record_ptr->inode_number,
                scan_cache_record_ptr->inode_generation);
        else
                printk("Fail to search : %lu %lu %lu\n",
                       scan_cache_key_ptr.dev_id,
                       scan_cache_key_ptr.inode_number,
                       scan_cache_key_ptr.inode_generation);
#endif
        if (scan_cache_record_ptr) {
                find_remove_lru_cache_record(scan_cache_record_ptr->time_stamp);
                remove_scan_cache_record(scan_cache_record_ptr);
                return true;
        }
        return false;
}

void purge_scan_cache(void)
{
        struct scan_cache_record *scan_cache_record_ptr = NULL;
        while(!RB_EMPTY_ROOT(&scan_cache_rb_tree)) {
                        scan_cache_record_ptr = RB_FIRST(&scan_cache_rb_tree,
                                        struct scan_cache_record, cache_rb_node);
                        printk("Removing : %lu %lu %u\n", scan_cache_record_ptr->dev_id,
                               scan_cache_record_ptr->inode_number,
                               scan_cache_record_ptr->inode_generation);
                        remove_scan_cache_record(scan_cache_record_ptr);
        }
}


void purge_lru_cache(void)
{
        struct lru_cache_record *lru_cache_record_ptr = NULL;
        while(!RB_EMPTY_ROOT(&lru_cache_rb_tree)) {
                        lru_cache_record_ptr = RB_FIRST(&lru_cache_rb_tree,
                                        struct lru_cache_record, lru_rb_node);
                        printk("Removing LRU: %lu %lu\n",
                               lru_cache_record_ptr->dev_id,
                               lru_cache_record_ptr->inode_number);
                        remove_lru_cache_record(lru_cache_record_ptr);
        }
}

void purge_cache(void)
{
        purge_scan_cache();
        purge_lru_cache();
}


void remove_cache_entry(int count)
{
        struct scan_cache_record *scan_cache_record_ptr = NULL;
        struct scan_cache_record_compare scan_cache_key_ptr;
        struct lru_cache_record *lru_cache_record_ptr = NULL;
        int remove_count = 0;

        if (count <= 0)
                goto out;

        while(!RB_EMPTY_ROOT(&lru_cache_rb_tree)) {

                 lru_cache_record_ptr = RB_FIRST(&lru_cache_rb_tree,
                                        struct lru_cache_record, lru_rb_node);
                 if (!lru_cache_record_ptr)
                         break;

                 printk("Removing Cache LRU: %lu %lu\n",
                        lru_cache_record_ptr->dev_id,
                        lru_cache_record_ptr->inode_number);

                 scan_cache_key_ptr.dev_id = lru_cache_record_ptr->dev_id;
                 scan_cache_key_ptr.inode_number = lru_cache_record_ptr->inode_number;
                 scan_cache_key_ptr.inode_generation = lru_cache_record_ptr->inode_generation;

                 scan_cache_record_ptr = find_scan_cache_record(&scan_cache_key_ptr);
                 if (!scan_cache_record_ptr)
                         break;

                 remove_lru_cache_record(lru_cache_record_ptr);
                 remove_scan_cache_record(scan_cache_record_ptr);
                 remove_count++;

                 if (remove_count >= count)
                         break;
         }

out:
        return;

}

int init_scan_cache(void)
{
        int ret = 0;
        ptr_scan_cache_slab = kmem_cache_create("scan_tee_cache",
                                             sizeof(struct scan_cache_record),
                                             0, SLAB_HWCACHE_ALIGN, 0);
        if (!ptr_scan_cache_slab) {
                printk("Fail to create cache for scan_cache_record\n");
                ret = -ENOMEM;
        }


        ptr_lru_cache_slab = kmem_cache_create("lru_tree_cache",
                                        sizeof(struct lru_cache_record),
                                        0, SLAB_HWCACHE_ALIGN, 0);
        if (!ptr_lru_cache_slab) {
                printk("Fail to create cache for ptr_lru_cache_slab\n");
                ret = -ENOMEM;
        }

        return ret;
}

void exit_scan_cache(void)
{

        //Test and remove only 2 entry
        remove_cache_entry(2);

        purge_cache();

        if (ptr_scan_cache_slab) {
                kmem_cache_destroy(ptr_scan_cache_slab);
                ptr_scan_cache_slab = NULL;
        }

        if (ptr_lru_cache_slab) {
                kmem_cache_destroy(ptr_lru_cache_slab);
                ptr_lru_cache_slab = NULL;
        }
}
