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
#include <linux/module.h>    // included for all kernel modules
#include <linux/kernel.h>    // included for KERN_INFO
#include <linux/init.h>      // included for __init and __exit macros
#include <linux/fs.h>
#include <linux/version.h>
#include <linux/mount.h>
#include <asm/current.h>
#include <linux/sched.h>
#include <linux/fs_struct.h>
#include <linux/slab.h>

#if (LINUX_VERSION_CODE >= KERNEL_VERSION(3,3,0))
#include <../fs/mount.h>
#endif

struct vfs_mount_list {
    struct vfsmount *mnt_data;
    struct list_head list;
};

static bool filesystem_name_compare(struct vfs_mount_list *mnt_point1,
                                 struct vfs_mount_list *mnt_point2)
{
        if (strcmp(mnt_point1->mnt_data->mnt_sb->s_type->name,
                   mnt_point2->mnt_data->mnt_sb->s_type->name) == 0)
                return true;
        else
                return false;
}

static bool vfs_mount_point_compare(struct vfs_mount_list *mnt,
                                 struct list_head *head)
{
        struct vfs_mount_list *tmp;
        struct list_head *list_current;
        bool add_record = false;
        list_for_each(list_current, head) {
                tmp = list_entry(list_current, struct vfs_mount_list, list);
                if (filesystem_name_compare(tmp, mnt)) {
                        add_record = true;
                        break;
                }
        }
        return add_record;
}

static int add_to_filesystem_record(struct vfsmount *mnt,
                                    struct list_head *head)
{
        struct vfs_mount_list *record_vfs_mount;
        int ret = 0;
        record_vfs_mount = (struct vfs_mount_list *)kmalloc \
                                (sizeof(struct vfs_mount_list), GFP_KERNEL);
        if (record_vfs_mount) {
                record_vfs_mount->mnt_data = mnt;
                if (!list_empty(head)) {
                        if (vfs_mount_point_compare(record_vfs_mount, head) == false)
                                list_add_tail(&record_vfs_mount->list, head);
                } else {
                        list_add_tail(&record_vfs_mount->list, head);
                }
        } else {
                printk("Fail to allocate the memory\n");
                ret  = -ENOMEM;
        }
        return ret;
}

#if (LINUX_VERSION_CODE < KERNEL_VERSION(3,3,0))
static struct vfsmount *get_root_mount_point(void)
#else
static struct mount *get_root_mount_point()
#endif
{
        struct vfsmount *root_mnt = NULL;
#if (LINUX_VERSION_CODE >= KERNEL_VERSION(2,6,38))
        rcu_read_lock();
        spin_lock(&current->fs->lock);
#else
        read_lock(&current->fs->lock);
        spin_lock(&dcache_lock);
#endif

#if (LINUX_VERSION_CODE < KERNEL_VERSION(2,6,27))
        for (root_mnt = current->fs->root_mnt;
             root_mnt != root_mnt->mnt_parent;
             root_mnt = root_mnt->mnt_parent)
#elif (LINUX_VERSION_CODE >= KERNEL_VERSION(3,3,0))
        root_mnt = current->fs->root.mnt;
        struct mount *mount_list = real_mount(root_mnt);
        for(; mount_list != mount_list->mnt_parent;
            mount_list = mount_list->mnt_parent)
#else
        for (root_mnt = current->fs->root.mnt;
             root_mnt != root_mnt->mnt_parent;
             root_mnt = root_mnt->mnt_parent)
#endif
                /* for loop is empty */ ;

#if (LINUX_VERSION_CODE >= KERNEL_VERSION(2,6,38))
        spin_unlock(&current->fs->lock);
        rcu_read_unlock();
#else
        spin_unlock(&dcache_lock);
        read_unlock(&current->fs->lock);
#endif


#if (LINUX_VERSION_CODE >= KERNEL_VERSION(3,3,0))
	return mount_list;
#else
	return root_mnt;
#endif
}

#if (LINUX_VERSION_CODE >= KERNEL_VERSION(3,3,0))
struct mount *child_mounted_filesystem(struct list_head *next)
#else
struct vfsmount *child_mounted_filesystem(struct list_head *next)
#endif
{
#if (LINUX_VERSION_CODE >= KERNEL_VERSION(3,3,0))
	struct mount *mnt_next;
        mnt_next = list_entry(next, struct mount, mnt_child);
#else
	struct vfsmount *mnt_next;
        mnt_next = list_entry(next, struct vfsmount, mnt_child);
#endif

	return mnt_next;
}


static int iterate_mount_point(struct list_head *head_list_point)
{
        struct vfsmount *mnt = NULL;
#if (LINUX_VERSION_CODE >= KERNEL_VERSION(3,3,0))
        struct mount *mnt_next = NULL;
        struct mount *mount_list = NULL;
#else
        struct vfsmount *root_mnt = NULL;
        struct vfsmount *mnt_next = NULL;
#endif
        struct list_head *next = NULL;
        struct list_head *head = NULL;

#if (LINUX_VERSION_CODE >= KERNEL_VERSION(3,3,0))
        mount_list = get_root_mount_point();
        if (mount_list == NULL) {
                printk("Error: Fail to get the rootmount\n");
                return 1;
        }
#else
        root_mnt = get_root_mount_point();
        if (root_mnt == NULL) {
                printk("Error: Fail to get the rootmount\n");
                return 1;
        }
#endif

#if (LINUX_VERSION_CODE >= KERNEL_VERSION(2,6,38))
        rcu_read_lock();
#endif

#if (LINUX_VERSION_CODE >= KERNEL_VERSION(3,3,0))
        mnt = mntget(&(mount_list->mnt));
#else
        mnt = mntget(root_mnt);
#endif

        do {
#if (LINUX_VERSION_CODE < KERNEL_VERSION(2,6,38))
                spin_lock(&dcache_lock);
#endif
                add_to_filesystem_record(mnt, head_list_point);
#if (LINUX_VERSION_CODE >= KERNEL_VERSION(3,3,0))
                head = &mount_list->mnt_mounts;
                next = mount_list->mnt_mounts.next;
#else
                head = &mnt->mnt_mounts;
                next = mnt->mnt_mounts.next;
#endif
                if (!list_empty(head)) {
                        mnt_next = child_mounted_filesystem(next);
                } else {

#if (LINUX_VERSION_CODE >= KERNEL_VERSION(3,3,0))
                        mnt_next = mount_list;
#else
                        mnt_next = mnt;
#endif

                        while (mnt_next != mnt_next->mnt_parent) {
                                next = mnt_next->mnt_child.next;
                                if (next != &mnt_next->mnt_parent->mnt_mounts) {
                                        break;
                                }
                                mnt_next = mnt_next->mnt_parent;
                        }

                        if (mnt_next == mnt_next->mnt_parent) {
                                mntput(mnt);
#if (LINUX_VERSION_CODE < KERNEL_VERSION(2,6,38))
                                spin_unlock(&dcache_lock);
#endif
                                break;
                        }
                        mnt_next = child_mounted_filesystem(next);
                }

#if (LINUX_VERSION_CODE >= KERNEL_VERSION(3,3,0))
                mntget(&(mnt_next->mnt));
                mntput(mnt);
                mount_list = mnt_next;
                mnt = &(mount_list->mnt);
#else
                mntget(mnt_next);
                mntput(mnt);
                mnt = mnt_next;
#endif

#if (LINUX_VERSION_CODE < KERNEL_VERSION(2,6,38))
                spin_unlock(&dcache_lock);
#endif
        } while (
#if (LINUX_VERSION_CODE >= KERNEL_VERSION(3,3,0))
                 mount_list
#else
                 mnt
#endif
                 );

#if (LINUX_VERSION_CODE >= KERNEL_VERSION(2,6,38))
        rcu_read_unlock();
#endif
        return 0;
}

static int __init filesystem_list_init(void)
{
        struct list_head *cur, *q;
        struct vfs_mount_list *tmp;
        struct vfs_mount_list file_record;
        INIT_LIST_HEAD(&file_record.list);
        iterate_mount_point(&file_record.list);

        printk(KERN_INFO "list: Printing list...\n");
        list_for_each(cur, &file_record.list) {
                tmp = list_entry(cur, struct vfs_mount_list , list);
                printk(KERN_INFO "list: %s\n",
                       tmp->mnt_data->mnt_sb->s_type->name );
        }

        list_for_each_safe(cur, q, &file_record.list) {
                tmp = list_entry(cur, struct vfs_mount_list, list);
                printk(KERN_INFO "Delete list: %s\n",
                       tmp->mnt_data->mnt_sb->s_type->name );
                list_del(cur);
                kfree(tmp);
        }

        return 0;    // Non-zero return means that the module couldn't be loaded.
}

static void __exit filesystem_list_cleanup(void)
{
        printk(KERN_INFO "Cleaning up module.\n");
}

module_init(filesystem_list_init);
module_exit(filesystem_list_cleanup);
MODULE_LICENSE("GPL");
MODULE_AUTHOR("Ashish");
MODULE_DESCRIPTION("module to print the mount filesystem");
