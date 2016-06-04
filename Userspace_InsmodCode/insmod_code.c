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

/* Sample code and work as insmod */
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <unistd.h>
#include <fcntl.h>
#include <sys/mman.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>
#include <errno.h>
#include <asm/unistd.h>

/* We use error numbers in a loose translation... */
static const char *moderror(int err)
{
        switch (err) {
        case ENOEXEC:
                return "Invalid module format";
        case ENOENT:
                return "Unknown symbol in module";
        case ESRCH:
                return "Module has wrong symbol version";
        case EINVAL:
                return "Invalid parameters";
        default:
                return strerror(err);
        }
}

static void *module_image_size(const char *filename, unsigned long *size)
{
        unsigned int max = 16384;
        int ret, fd, err_save;
        void *buffer;

        fd = open(filename, O_RDONLY, 0);

        if (fd < 0)
                return NULL;

        buffer = malloc(max);
        if (!buffer)
                goto out_error;

        *size = 0;
        while ((ret = read(fd, buffer + *size, max - *size)) > 0) {
                *size += ret;
                if (*size == max) {
                        void *p;

                        p = realloc(buffer, max *= 2);
                        if (!p)
                                goto out_error;
                        buffer = p;
                }
        }
        if (ret < 0)
                goto out_error;

        close(fd);
        return buffer;

out_error:
        err_save = errno;
        free(buffer);
        close(fd);
        errno = err_save;
        return NULL;
}


int main(int argc, char *argv[])
{
        unsigned int i;
        long int ret;
        unsigned long len;
        void *file;
        char *filename, *options = strdup("");
        char *progname = argv[0];

        if (!options) {
                fprintf(stderr,
                        "insmod: can't allocate memory: %s\n",
                        strerror(errno));
                exit(1);
        }


        filename = argv[1];

        for (i = 2; i < argc; i++) {
                options = realloc(options,
                                  strlen(options) + 1 + strlen(argv[i]) + 1);
                if (!options) {
                        fprintf(stderr,
                                "insmod: can't allocate memory: %s\n",
                                strerror(errno));
                        exit(1);
                }
                strcat(options, argv[i]);
                strcat(options, " ");
        }

        file = module_image_size(filename, &len);
        if (!file) {
                fprintf(stderr, "insmod: can't read '%s': %s\n",
                        filename, strerror(errno));
                exit(1);
        }

        ret = init_module(file, len, options);
        if (ret != 0) {
                fprintf(stderr, "insmod: error inserting '%s': %li %s\n",
                        filename, ret, moderror(errno));
        }
        free(file);

        if (ret != 0)
                exit(1);
        exit(0);
}
