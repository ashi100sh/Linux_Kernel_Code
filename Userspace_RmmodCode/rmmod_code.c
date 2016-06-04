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

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <errno.h>
#include <fcntl.h>
#include <asm/unistd.h>
#include <stdarg.h>
#include <getopt.h>
#include <syslog.h>

extern long delete_module(const char *, unsigned int);

/*
 * Get the basename in a pathname.
 * Unlike the standard implementation, this does not copy the string.
 */
char *basename(const char *path)
{
	const char *base = strrchr(path, '/');
	if (base)
		return (char *) base + 1;
	return (char *) path;
}

/*
 * Convert filename to the module name.  Works if filename == modname, too.
 */
void filename2modname(char *modname, const char *filename)
{
	const char *afterslash;
	unsigned int i;

	afterslash = basename(filename);

	/* Convert to underscores, stop at first . */
	for (i = 0; afterslash[i] && afterslash[i] != '.'; i++) {
		if (afterslash[i] == '-')
			modname[i] = '_';
		else
			modname[i] = afterslash[i];
	}
	modname[i] = '\0';
}


/* If we can check usage without entering kernel, do so. */
static int check_usage(const char *modname)
{
	FILE *module_list;
	char line[10240], name[64];
	unsigned long size, refs;
	int scanned;

	module_list = fopen("/proc/modules", "r");
	if (!module_list) {
		if (errno == ENOENT) /* /proc may not be mounted. */
			return 0;
		printf("can't open /proc/modules: %s\n", strerror(errno));
	}

	while (fgets(line, sizeof(line)-1, module_list) != NULL) {
		if (strchr(line, '\n') == NULL) {
			printf("V. v. long line broke rmmod.\n");
			exit(1);
		}

		scanned = sscanf(line, "%s %lu %lu", name, &size, &refs);
		if (strcmp(name, modname) != 0)
			continue;

		if (scanned < 2)
			printf("Unknown format in /proc/modules: %s\n", line);

		if (scanned == 2)
			printf("Kernel does not have unload support.\n");

		if (refs != 0) {
                        printf("Cannot remove the module, %s is in use\n", line);
                }
		goto out;
	}
	error("Module %s does not exist in /proc/modules\n", modname);
	refs = 1;
 out:
	fclose(module_list);
	return (refs == 0) ? 0 : -EINVAL;
}

static int rmmod(const char *path, int flags)
{
	long ret;
	char name[strlen(path) + 1];

	filename2modname(name, path);

        if ((flags & O_NONBLOCK) && !(flags & O_TRUNC)) {
                if (check_usage(name) != 0)
                        return 1;
        }


	ret = delete_module(name, flags);
	if (ret != 0)
		printf("Removing '%s': %s\n", name, strerror(errno));
	return ret;
}


int main(int argc, char *argv[])
{
	/* O_EXCL so kernels can spot old rmmod versions */
	unsigned int flags = O_NONBLOCK|O_EXCL;
	int i, opt, all = 0;
	int ret, err;

	err = 0;
	/* remove each specified module */
        ret = rmmod(argv[1], flags);
        if (ret != 0)
                err = 1;

	exit(err);
}
