obj-m += rbtree_mod.o

rbtree_mod-y := src/rbtree_test.o
rbtree_mod-y += src/cache.o

ccflags-y := -I$(PWD)/include

all:
	make -C /lib/modules/$(shell uname -r)/build M=$(PWD) modules

clean:
	make -C /lib/modules/$(shell uname -r)/build M=$(PWD) clean

