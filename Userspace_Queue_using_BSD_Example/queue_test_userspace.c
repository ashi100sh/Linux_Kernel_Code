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

/* Refer for more details:
http://stackoverflow.com/questions/22315213/minimal-example-of-tailq-usage-out-of-sys-queue-h-library
http://blog.jasonish.org/2006/08/19/tailq-example/
*/

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>

#include "queue.h"

struct foo {
        TAILQ_ENTRY(foo) tailq;
        int datum;
};

TAILQ_HEAD(fooq, foo);

int main()
{
        struct foo *p, *tmp;
        struct foo *data1, *data2, *data3;
        struct fooq q = TAILQ_HEAD_INITIALIZER(q);
        //struct fooq q;
        //TAILQ_INIT(&q);

        data1 = malloc(sizeof(*data1));
        data1->datum = 1;

        data2 = malloc(sizeof(*data2));
        data2->datum = 2;

        data3 = malloc(sizeof(*data3));
        data3->datum = 3;


        TAILQ_INSERT_TAIL(&q, data1, tailq);
        TAILQ_INSERT_TAIL(&q, data2, tailq);
        TAILQ_INSERT_TAIL(&q, data3, tailq);

        TAILQ_FOREACH(p, &q, tailq)
        {
                printf(" %d\n", p->datum);
        }
        puts("");

        printf("Removing the Entry from Queue\n");

#if 0
        while (!TAILQ_EMPTY(&q))
        {
                p = TAILQ_FIRST(&q);
                printf("Removing : %d\n", p->datum);
                TAILQ_REMOVE(&q, p, tailq);
        }
#endif

        //tmp pointing to next
        TAILQ_FOREACH_SAFE(p, tmp, & q, tailq)
        {
                printf("Removing %d\n", p->datum);
                TAILQ_REMOVE(&q , p , tailq);
                free(p);
        }


}
