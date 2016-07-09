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

#include <linux/version.h>
#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/init.h>
#include <linux/proc_fs.h>
#include <linux/slab.h>
#include <asm/uaccess.h>

#define ROOT_PROCFS     "ashish" // Proc: /proc/ashish/
#define LONG_BUF_LEN 21 /* Enough to hold 2^64 plus a terminator */

static struct proc_dir_entry *proc_dir_root;

typedef  ssize_t (proc_read_proto)(struct file *file, char __user *userbuf,
		size_t count, loff_t *ppos);
typedef  ssize_t (proc_write_proto)(struct file *file,
		const char __user *userbuf,
		size_t count, loff_t *ppos);

long test_proc = 0;

int init_procfs(void)
{
	proc_dir_root = proc_mkdir(ROOT_PROCFS, NULL);
	if (proc_dir_root) {
		printk("Created new procfs directory %p\n", proc_dir_root);
	} else {
		printk(KERN_INFO "Error creating proc entry");
		return -ENOMEM;
	}
	return 0;
}

void term_procfs(void)
{
	if (proc_dir_root) {
		remove_proc_entry(ROOT_PROCFS ,NULL);
	}
}

struct proc_dir_entry *PF_getDir(void)
{
	return proc_dir_root;
}

#if (LINUX_VERSION_CODE >= KERNEL_VERSION(3,10,0))
int dummy_proc_open(struct inode *inode, struct file *file)
{
	return 0;
}

ssize_t procfs_read_long(struct file *file, char __user *userbuf,
		size_t count, loff_t *ppos)
{
	long *longVal = PDE_DATA(file_inode(file));
	char *data = NULL;
	int ret = 0;

	if (*ppos >= count)
		goto out;

	data = kzalloc(LONG_BUF_LEN, GFP_KERNEL);
	if (data) {
		snprintf(data, LONG_BUF_LEN, "%ld\n", *longVal); 
		if (copy_to_user(userbuf, data, LONG_BUF_LEN)) {
			goto out;
		}
		*ppos += count;
		ret = count;
		kfree(data);
	}
out:
	return ret;
}

ssize_t procfs_write_long(struct file *file,
		const char __user *userbuf,
		size_t count, loff_t *ppos)
{
	long *longVal = PDE_DATA(file_inode(file));
	char *data = NULL;
	char *pend = NULL;
	int intVal = 0;
	int ret = 0;

	data = kzalloc(LONG_BUF_LEN, GFP_KERNEL);
	if (data) {
		if (count > sizeof(data) -1) {
			kfree(data);
			ret = -EINVAL;
			goto out;
		}

		if (copy_from_user(data, userbuf, count)) {
			kfree(data);
			ret = -EINVAL;
			goto out;
		}

		data[count]= '\0';
		intVal = simple_strtol(data, &pend, 10);
		*longVal = intVal;
		kfree(data);
		ret = count;
	} else {
		ret = -ENOMEM;
	}

out: 
	return ret;
}

#endif

#if (LINUX_VERSION_CODE >= KERNEL_VERSION(3,10,0))
static void procfs_create_entry(const char *name, void *data, struct file_operations *proto_proc_fops,
		proc_read_proto *read , proc_write_proto *write)
{
	struct proc_dir_entry *entry;
	mode_t mode;

	mode = S_IFREG;
	if (read)
		mode |= S_IRUSR;
	if (write)
		mode |= S_IWUSR;

	proto_proc_fops->owner = THIS_MODULE;
	proto_proc_fops->open = dummy_proc_open;
	proto_proc_fops->read = read;
	proto_proc_fops->llseek = no_llseek;
	proto_proc_fops->write = write; 
	entry = proc_create_data(name, mode, proc_dir_root,
			proto_proc_fops, data);
	if (!entry) {
		printk("Fail to create the proc entry for %s\n", name);
	}
}
#else
void procfs_create_entry(const char *name, void *data,
		read_proc_t *read, write_proc_t *write)
{
	struct proc_dir_entry *entry;
	mode_t mode;

	mode = S_IFREG;
	if (read)
		mode |= S_IRUSR;
	if (write)
		mode |= S_IWUSR;

	entry = create_proc_entry(name, mode, proc_dir_root);
	if (entry) {
		entry->read_proc = read;
		entry->write_proc = write;
		entry->data = data;
	} else {
		printk("Fail to create the proc entry for %s\n", name);
	}
}

#endif


void procfs_remove_entry(const char *name)
{
	if (proc_dir_root)
		remove_proc_entry(name, proc_dir_root);
}


#if (LINUX_VERSION_CODE < KERNEL_VERSION(3,10,0))
int procfs_read_long(char *page, char **start, off_t off,
		int count, int *eof, void *data)
{
	int length;

	length = snprintf(page, PAGE_SIZE, "%ld\n", *(long *)data);
	if (length <= off + count)
		*eof = 1;
	*start = page + off;
	length -= off;

	return (length < 0) ? 0 : length;
}

int procfs_write_long(struct file *file, const char *buffer,
		unsigned long count, void *data)
{
	long longVal;
	char *pend;
	char buff_long[LONG_BUF_LEN];

	if (count > sizeof(buff_long) - 1)
		return -EINVAL;
	if (copy_from_user(buff_long, buffer, count))
		return -EINVAL;

	buff_long[count] = '\0';
	longVal = simple_strtol(buff_long, &pend, 10);

	*(long *)data = longVal;

	return count;
}
#endif

#if (LINUX_VERSION_CODE >= KERNEL_VERSION(3,10,0))
struct file_operations proto_proc_fops;
#endif

static int __init procfs_init(void)
{
	init_procfs();
#if (LINUX_VERSION_CODE >= KERNEL_VERSION(3,10,0))
	procfs_create_entry("Test_PROCFS", &test_proc, &proto_proc_fops, procfs_read_long, procfs_write_long);
#else
	procfs_create_entry("Test_PROCFS", &test_proc, procfs_read_long, procfs_write_long);
#endif
	return 0;
}

static void __exit procfs_cleanup(void)
{
	procfs_remove_entry("Test_PROCFS");
	term_procfs();
}

module_init(procfs_init);
module_exit(procfs_cleanup);

MODULE_LICENSE("GPL");
MODULE_AUTHOR("Ashish Singh");
MODULE_DESCRIPTION("Procfs Test Program");
