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

#include <linux/kthread.h>
#include <linux/module.h>
#include <linux/slab.h>
#include <linux/fs.h>
#include <linux/version.h>
#include <linux/file.h>
#include <linux/mount.h>
#include <linux/syscalls.h>
/*!
 * \brief Create the file descriptor which is going to send to userspace
 * \param filp is file struct pointer
 * \return file descriptor in case successful otherwise -1
 */
static int create_fd(struct file *filp)
{
        int client_fd;
        struct dentry *dentry = NULL;
        struct vfsmount *mnt = NULL;
        struct file *new_file = NULL;
        client_fd = get_unused_fd();
        if (client_fd < 0) {
                return -1;
        }
        else {
                /* dget use to get a reference to a dentry and increment the
                 * reference count */
                dentry = dget(filp->f_path.dentry);
                /* Increment mnt count */
                mnt = mntget(filp->f_path.mnt);
                if (dentry && mnt)
#if (LINUX_VERSION_CODE >= KERNEL_VERSION(3,6,0))
                        new_file = dentry_open(&filp->f_path,
                                               filp->f_flags, current_cred());
#else
                        new_file = dentry_open(dentry, mnt,
                                               filp->f_flags, current_cred());
#endif
                else
                        new_file = ERR_PTR(-EOVERFLOW);

                if (IS_ERR(new_file)) {
                        put_unused_fd(client_fd);
                        client_fd = PTR_ERR(new_file);
                } else {
                        /* Assign client_fd i.e file descriptor to file
                         * descriptor table of file structure */
                        fd_install(client_fd, new_file);
                }
        }
        return client_fd;
}

static int __init init_createfd_test(void)
{
        int fd;
        struct file *filp = NULL;
        // Tested in RHEL 6.x 
        //filp = filp_open("/proc/stat",  O_RDONLY);
        filp = filp_open("/proc/stat",0, O_RDONLY);
        if (filp) {
                fd = create_fd(filp);
                printk("New FD:%d\n", fd);
                if (fd != -1)
                        sys_close(fd);
        }
        return 0;
}
static void __exit exit_createfd_test(void)
{
        printk("Exit module\n");
}
module_init(init_createfd_test);
module_exit(exit_createfd_test);

