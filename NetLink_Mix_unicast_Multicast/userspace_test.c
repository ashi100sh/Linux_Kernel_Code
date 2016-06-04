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

#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/socket.h>
#include <linux/netlink.h>


/* Protocol family, consistent in both kernel prog and user prog. */
#define NETLINK_USER 31
/* Multicast group, consistent in both kernel prog and user prog. */
#define MYMGRP 21
#define MAX_PAYLOAD 1024 /* maximum payload size*/

int registration_kernel()
{

        struct sockaddr_nl src_addr, dest_addr;
        struct nlmsghdr *nlh = NULL;
        struct iovec iov;

        int sock_fd;

        sock_fd = socket(PF_NETLINK, SOCK_RAW, NETLINK_USER);
        if (sock_fd < 0)
                return -1;

        memset(&src_addr, 0, sizeof(src_addr));
        src_addr.nl_family = AF_NETLINK;
        src_addr.nl_pid = getpid(); /* self pid */

        bind(sock_fd, (struct sockaddr *)&src_addr, sizeof(src_addr));

        memset(&dest_addr, 0, sizeof(dest_addr));
        memset(&dest_addr, 0, sizeof(dest_addr));
        dest_addr.nl_family = AF_NETLINK;
        dest_addr.nl_pid = 0; /* For Linux Kernel */
        dest_addr.nl_groups = 0; /* unicast */

        nlh = (struct nlmsghdr *)malloc(NLMSG_SPACE(MAX_PAYLOAD));
        memset(nlh, 0, NLMSG_SPACE(MAX_PAYLOAD));
        nlh->nlmsg_len = NLMSG_SPACE(MAX_PAYLOAD);
        nlh->nlmsg_pid = getpid();
        nlh->nlmsg_flags = 0;

        strcpy(NLMSG_DATA(nlh), "Hello");

        iov.iov_base = (void *)nlh;
        iov.iov_len = nlh->nlmsg_len;
        struct msghdr msg;

        msg.msg_name = (void *)&dest_addr;
        msg.msg_namelen = sizeof(dest_addr);
        msg.msg_iov = &iov;
        msg.msg_iovlen = 1;

        printf("Sending message to kernel\n");
        sendmsg(sock_fd, &msg, 0);
        printf("Waiting for message from kernel\n");

        struct msghdr msg1;

        /* Read message from kernel */
        recvmsg(sock_fd, &msg1, 0);
        printf("Received message payload: %s\n", NLMSG_DATA(nlh));
        close(sock_fd);
        return 0;
}


int open_netlink_mulicast(void)
{
        int sock;
        struct sockaddr_nl addr;
        int group = MYMGRP;

        sock = socket(AF_NETLINK, SOCK_RAW, NETLINK_USER); /*PF_NETLINK is also working */
        if (sock < 0) {
                printf("sock < 0.\n");
                return sock;
        }

        memset((void *) &addr, 0, sizeof(addr));
        addr.nl_family = AF_NETLINK; /*PF_NETLINK  is also working */
        addr.nl_pid = getpid();
        /* This doesn't work for some reason. See the setsockopt() below. */
        /* addr.nl_groups = MYMGRP; */

        if (bind(sock, (struct sockaddr *) &addr, sizeof(addr)) < 0) {
                printf("bind < 0.\n");
                return -1;
        }

        /*
         * 270 is SOL_NETLINK. See
         * http://lxr.free-electrons.com/source/include/linux/socket.h?v=4.1#L314
         * and
         * http://stackoverflow.com/questions/17732044/
         */
        if (setsockopt(sock, 270, NETLINK_ADD_MEMBERSHIP, &group,
                       sizeof(group)) < 0) {
                printf("setsockopt < 0\n");
                return -1;
        }

        return sock;
}

void read_event_from_kernel(int sock)
{
        struct sockaddr_nl nladdr;
        struct msghdr msg;
        struct iovec iov;
        char buffer[65536];
        int ret;

        iov.iov_base = (void *) buffer;
        iov.iov_len = sizeof(buffer);
        msg.msg_name = (void *) &(nladdr);
        msg.msg_namelen = sizeof(nladdr);
        msg.msg_iov = &iov;
        msg.msg_iovlen = 1;

        printf("Ok, listening.\n");
        ret = recvmsg(sock, &msg, 0);
        if (ret < 0)
                printf("ret < 0.\n");
        else
                printf("Received message payload: %s\n",
                       NLMSG_DATA((struct nlmsghdr *) &buffer));
}


int main()
{
        int nls;
        /* Unicast registration mechanism is used */
        nls = registration_kernel();
        if (nls < 0)
                return nls;

        /* Open the socket and set attritube to perform the multicast group receiver */
        nls = open_netlink_mulicast();
        if (nls < 0)
                return nls;

        /* Read the event from kernel*/
        while (1)
                read_event_from_kernel(nls);

        return 0;
}
