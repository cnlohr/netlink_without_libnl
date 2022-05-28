all : using_libnl without_libnl

CFLAGS:= -Os -s

using_libnl : using_libnl.c
	gcc $(CFLAGS) -o $@ $^ -lnl-3 -I/usr/include/libnl3 -lnl-genl-3

without_libnl : without_libnl.c
	gcc $(CFLAGS) -o $@ $^

clean :
	rm -rf *.o *~ using_libnl without_libnl


