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
#include <sys/socket.h>
#include <linux/netlink.h>
#include <unistd.h>

/* Protocol family, consistent in both kernel prog and user prog. */
#define MYPROTO NETLINK_USERSOCK
/* Multicast group, consistent in both kernel prog and user prog. */
#define MYMGRP 21

int open_netlink(void)
{
    int sock;
    struct sockaddr_nl addr;
    int group = MYMGRP;

    sock = socket(AF_NETLINK, SOCK_RAW, MYPROTO); /*PF_NETLINK is also working */
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
    if (setsockopt(sock, 270, NETLINK_ADD_MEMBERSHIP, &group, sizeof(group)) < 0) {
        printf("setsockopt < 0\n");
        return -1;
    }

    return sock;
}

void read_event(int sock)
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
        printf("Received message payload: %s\n", NLMSG_DATA((struct nlmsghdr *) &buffer));
}

int main(int argc, char *argv[])
{
    int nls;

    nls = open_netlink();
    if (nls < 0)
        return nls;

    while (1)
        read_event(nls);

    return 0;
}
