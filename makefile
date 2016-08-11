all:
	gcc -shared -fpic malloc.c -o malloc.so
