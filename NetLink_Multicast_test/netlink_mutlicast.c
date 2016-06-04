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
Refer:http://stackoverflow.com/questions/22691305/multicast-from-kernel-to-user-space-via-netlink-in-c
*/

#include <linux/module.h>
#include <net/sock.h>
#include <linux/netlink.h>
#include <linux/skbuff.h>

/* Protocol family, consistent in both kernel prog and user prog. */
#define MYPROTO NETLINK_USERSOCK
/* Multicast group, consistent in both kernel prog and user prog. */
#define MYGRP 21
struct sock *nl_sk = NULL;

static void send_to_user(void)
{
        struct sk_buff *skb;
        struct nlmsghdr *nlh;
        char *msg = "Hello from kernel";
        int msg_size = strlen(msg) + 1;
        int res;

        pr_info("Creating skb.\n");
        skb = nlmsg_new(NLMSG_ALIGN(msg_size + 1), GFP_KERNEL);
        if (!skb)
        {
                printk("Allocation failure.\n");
                return;
        }

        nlh = nlmsg_put(skb, 0, 1, NLMSG_DONE, msg_size + 1, 0);
        strcpy(nlmsg_data(nlh), msg);

        printk("Sending skb.\n");
        res = nlmsg_multicast(nl_sk, skb, 0, MYGRP, GFP_KERNEL);
        if (res < 0)
                printk("nlmsg_multicast() error: %d\n", res);
        else
                printk("Success.\n");
}


static int __init hello_init(void)
{
        printk("Entering: %s\n", __FUNCTION__);
        nl_sk = netlink_kernel_create(&init_net, NETLINK_USERSOCK, 0,
                                      NULL, NULL, THIS_MODULE);
        if (!nl_sk)
        {
                printk(KERN_ALERT "Error creating socket.\n");
                return -10;
        }
        send_to_user();
        netlink_kernel_release(nl_sk);
        return 0;
}

static void __exit hello_exit(void)
{
        printk(KERN_INFO "exiting hello module\n");
}

module_init(hello_init);
module_exit(hello_exit);
MODULE_LICENSE("GPL");
