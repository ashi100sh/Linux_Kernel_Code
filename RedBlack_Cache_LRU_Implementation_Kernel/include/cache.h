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
#ifndef _SCAN_CACHE_H
#define _SCAN_CACHE_H
#include <linux/rbtree.h>
#include <linux/time.h>

#define MAX_ENTRIES_CACHE (64*1024)
#define PURGE_ENTRIES_CACHE 512

#define container_of_or_null(ptr, type, member)                          \
({                                                                       \
	typeof(ptr) _ptr = ptr;                                          \
	_ptr ? container_of(_ptr, type, member) : NULL;                  \
})

#define RB_INSERT(root, new, member, cmp)                                \
({                                                                       \
         __label__ dup;                                                  \
         struct rb_node **n = &(root)->rb_node, *parent = NULL;          \
         typeof(new) this;                                               \
         int res, ret = -1;                                              \
                                                                         \
         while (*n) {                                                    \
                 parent = *n;                                            \
                 this = container_of(*n, typeof(*(new)), member);        \
                 res = cmp(new, this);                                   \
                 if (!res)                                               \
                         goto dup;                                       \
                 n = res < 0                                             \
                         ? &(*n)->rb_left                                \
                         : &(*n)->rb_right;                              \
         }                                                               \
                                                                         \
         rb_link_node(&(new)->member, parent, n);                        \
         rb_insert_color(&(new)->member, root);                          \
         ret = 0;                                                        \
dup:                                                                     \
         ret;                                                            \
})

/* search_record -> searched entry of  RB tree store in this
 * searching_key -> It is key Use for searching in RB Tree
 */
#define RB_SEARCH(root, search_record, searching_key, member, cmp)         \
({                                                                       \
         struct rb_node *n = (root)->rb_node;                            \
         typeof(search_record) this;                                      \
         int res;                                                        \
                                                                         \
         while (n) {                                                     \
                 this = container_of(n, typeof(*(search_record)), member);\
                 res = cmp(searching_key, this);                          \
                 if (!res) {                                             \
                 /* Found the search entry */				 \
                         search_record = this;                            \
                         break;                                          \
                 }                                                       \
                 n = res < 0                                             \
                         ? n->rb_left                                    \
                         : n->rb_right;                                  \
         }                                                               \
})

#define RB_REMOVE(root, removeRecord, member)                            \
({                                                                       \
        rb_erase(&removeRecord->member, root);                           \
})                                                                       \


#define RB_REPLACE(root, old, new, member)                               \
({                                                                       \
        rb_replace_node(&old->member, &new->member, root);               \
})                                                                       \

#define RB_FIRST(root, type, member)                                     \
	container_of_or_null(rb_first(root), type, member)

#define RB_LAST(root, type, member)                                      \
	container_of_or_null(rb_last(root), type, member)

#define RB_NEXT(ptr, member)                                             \
	container_of_or_null(rb_next(&(ptr)->member), typeof(*ptr), member)

#define RB_PREV(ptr, member)                                             \
	container_of_or_null(rb_prev(&(ptr)->member), typeof(*ptr), member)

extern struct rb_root scan_cache_rb_tree;

enum scan_state
{
        STATE_UNKNOWN,
        STATE_CLEAN,
        STATE_INFECTED,
};

struct scan_cache_record {
        dev_t dev_id;
        unsigned long inode_number;
        __u32 inode_generation;
        enum scan_state status; //enum ??
        struct timespec time_stamp;
        struct rb_node cache_rb_node;
};

struct scan_cache_record_compare {
        dev_t dev_id;
        unsigned long inode_number;
        __u32 inode_generation;
};

struct lru_cache_record {
        struct timespec time_stamp;
        dev_t dev_id;
        unsigned long inode_number;
        __u32 inode_generation;
        struct rb_node lru_rb_node;
};

struct lru_cache_record_compare{
        struct timespec time_stamp;
};

//Testing:
//struct file in reality
struct fileInfo {
        dev_t dev_id;
        unsigned long inode_number;
        __u32 inode_generation;
};

int insert_scan_cache_record(struct fileInfo *filp);
bool find_remove_scan_cache_record(struct fileInfo *filp);
void purge_scan_cache(void);
int init_scan_cache(void);
void exit_scan_cache(void);

#endif //_SCAN_CACHE_H
