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
/*
Refer: http://stackoverflow.com/questions/3299386/how-to-use-netlink-socket-to-communicate-with-a-kernel-module
*/

#include <linux/module.h>
#include <net/sock.h> 
#include <linux/netlink.h>
#include <linux/skbuff.h> 
#include <linux/kthread.h> //kthread

#define NETLINK_USER 31

/* Protocol family, consistent in both kernel prog and user prog. */
#define MYPROTO NETLINK_USERSOCK
/* Multicast group, consistent in both kernel prog and user prog. */
#define MYGRP 21

struct _Response
{
        int index;
        int dataSize;     /* ammount of data in bytes of the response */
        char data[4096];
        int test2;
        int test3;
        int test4;
};
typedef struct _Response Response;


struct multicast_mesg
{
        char name[1024];
        char process[1024];
};


struct sock *nl_sk = NULL;
struct task_struct *netlinkThread = NULL;

/*
 * Start sending the multicast message from kernel to userspace without waiting acknowledgement
 */
static int multicast_start(void *data)
{

        printk(KERN_INFO "Entry inside  %s\n", __FUNCTION__);

        allow_signal(SIGKILL);
        allow_signal(SIGTERM);


        while(1)
        {
                struct sk_buff *skb;
                struct nlmsghdr *nlh;
                struct multicast_mesg *msg = kmalloc(
                                sizeof(struct multicast_mesg), GFP_KERNEL);
                int msg_size = sizeof(struct multicast_mesg);
                int res;

                strcpy(msg->name, "Hello From Kernel");
                //Can add the \0 logic
                strcpy(msg->process, "World of NetLink");

                pr_info("------ Creating skb. -------:%d, %s, %s\n",
                        msg_size, msg->name, msg->process);

                skb = nlmsg_new(NLMSG_ALIGN(msg_size), GFP_KERNEL);
                if (!skb)
                {
                        printk("Allocation failure.\n");
                        return -1;
                }


                nlh = nlmsg_put(skb, 0, 1, NLMSG_DONE, msg_size, 0);
                memcpy(nlmsg_data(nlh), msg, msg_size);

                printk("Sending skb.\n");
                res = nlmsg_multicast(nl_sk, skb, 0, MYGRP, GFP_KERNEL);
                if (res < 0)
                        printk("nlmsg_multicast() error: %d , no one receive\n",
                                                res);
                else
                        printk("Success the message.\n");


                kfree((void*)msg);
                if (kthread_should_stop())
                {
                        break;
                }

                msleep(1000);

                printk("-----------------------------\n");
        }

        return 0;

}

/*
 * Callback invoked when userspace socket send messge to kernel
 */
static void callback_invoked_connect_userspace(struct sk_buff *skb)
{
        struct nlmsghdr *nlh;
        int pid;
        struct sk_buff *skb_out;
        int msg_size;
        int res;
        Response *req;

        printk(KERN_INFO "Entering: %s\n", __FUNCTION__);

        msg_size= sizeof(Response);

        nlh=(struct nlmsghdr*)skb->data;

        req = (Response*)NLMSG_DATA(nlh);

        printk("Recieved from Userspace:\n");
        printk("index %d\n", req->index);
        printk("dataSize %d\n", req->dataSize);
        printk("data: %s\n", req->data);
        printk("test2 %d\n", req->test2);
        printk("test3 %d\n", req->test3);
        printk("test4 %d\n", req->test4);

        pid = nlh->nlmsg_pid; /*pid of sending process */

        skb_out = nlmsg_new(msg_size,0);

        if(!skb_out)
        {

                printk(KERN_ERR "Failed to allocate new skb\n");
                return;

        }
        nlh=nlmsg_put(skb_out,0,0,NLMSG_DONE,msg_size,0);
        NETLINK_CB(skb_out).dst_group = 0; /* not in mcast group */

        req->index *= 100;
        req->dataSize *= 100;
        strcpy(req->data , "Data from Kernel");
        req->test2 *= 100;
        req->test3 *= 100;
        req->test4 *= 100;

        printk("Sending to Userspace:\n");
        printk("index %d\n", req->index);
        printk("dataSize %d\n", req->dataSize);
        printk("data: %s\n", req->data);
        printk("test2 %d\n", req->test2);
        printk("test3 %d\n", req->test3);
        printk("test4 %d\n", req->test4);

        memcpy(NLMSG_DATA(nlh), req, sizeof(Response));

        res=nlmsg_unicast(nl_sk,skb_out,pid);

        if(res<0)
        {
                printk(KERN_INFO "Error while sending bak to user\n");
                return;
        }


        /* Create the kernel thread , which have start multicast messaging */
        if (netlinkThread == NULL)
        {
                netlinkThread = kthread_run(&multicast_start, NULL, "Multicast");
                if (netlinkThread == NULL)
                        return;
        }
}


/*
 * Create the netlink
 */
static int __init netlink_test_init(void)
{
        printk("Entering: %s\n", __FUNCTION__);


        nl_sk = netlink_kernel_create(&init_net, NETLINK_USER, 0,
                        callback_invoked_connect_userspace, NULL, THIS_MODULE);
        if (!nl_sk)
        {
                printk(KERN_ALERT "Error creating socket.\n");
                return -10;
        }
        return 0;
}

static void __exit netlink_test_exit(void)
{

        if (netlinkThread)
                kthread_stop(netlinkThread);

        msleep(1000);

        if (nl_sk)
                netlink_kernel_release(nl_sk);

        printk(KERN_INFO "exiting hello module\n");
}

module_init(netlink_test_init);
module_exit(netlink_test_exit);
MODULE_LICENSE("GPL");
