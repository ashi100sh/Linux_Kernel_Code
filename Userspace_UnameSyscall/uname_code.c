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
 */

/*
 * code sample for get the system infomation
 * such as: sysname, nodename, release, version, machine.
 */

#include<stdio.h>
#include<unistd.h>
#include<stdlib.h>
#include<errno.h>
#include<string.h>
#include<sys/utsname.h>

int main(int argc,char**argv)
{
        int z;
        struct utsname u_name;
        z = uname(&u_name);
        if(z == -1)
        {
                fprintf(stderr,"%s:uname(2)\n",strerror(errno));
                exit (1);
        }

        printf("System infomation: \n");
        printf(" sysname[] = %s\n", u_name.sysname);
        printf(" nodename[] = %s\n", u_name.nodename);
        printf(" release[] = %s\n", u_name.release);
        printf(" version[] = %s\n", u_name.version);
        printf(" machine[] = %s\n", u_name.machine);

        return 0;
}
