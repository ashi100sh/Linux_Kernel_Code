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

/* Program read the /boot/System-map-`uname -r` and Display the field of
 * sys_call_table symbol field
 */
#include<stdio.h>
#include<unistd.h>
#include<stdlib.h>
#include<errno.h>
#include<string.h>
#include <linux/limits.h>
#include<sys/utsname.h>

int main()
{
        int err;
        FILE *file;
        struct utsname u_name;
        char sysmapFile[PATH_MAX];
        char line[1024];
        char *psearch;

        err = uname(&u_name);
        if (err == -1)
        {
                fprintf(stderr,"%s:uname(2)\n",strerror(errno));
                exit (1);
        }

        memset(sysmapFile, 0, PATH_MAX);
        strcpy(sysmapFile, "/boot/System.map-");
        strcat(sysmapFile, u_name.release);
        printf("%s\n", sysmapFile);
        file = fopen(sysmapFile, "r");
        if (!file) {
                printf("Fail to open the system.map file\n");
                exit(1);
        }

        while (fgets(line, sizeof(line)-1, file) != NULL) {
                psearch = strstr(line, "sys_call_table");
                if (psearch == NULL) {
                        continue;
                } else {
                        printf("%s", line);
                        char *saveptr;
                        char *spliter_element;
                        spliter_element = strtok_r(line, " ", &saveptr);
                        while ( spliter_element != NULL) {
                                printf("%s\n", spliter_element);
                                spliter_element = strtok_r(NULL, " ", &saveptr);
                        }
                        break;
                }
        }
        fclose(file);
}
