all : using_libnl without_libnl

using_libnl : using_libnl.c
	gcc -O0 -g -o $@ $^ -lnl-3 -I/usr/include/libnl3 -lnl-genl-3

without_libnl : without_libnl.c
	gcc -O0 -g -o $@ $^

clean :
	rm -rf *.o *~ using_libnl without_libnl


