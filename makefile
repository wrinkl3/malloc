all:
	gcc -shared -fpic malloc.c -o my_malloc.so
