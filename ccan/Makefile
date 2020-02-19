CC=gcc
 
CFLAGS = -Wall -O3
 
INCLUDE = -I.

LIB_DEST = ../lib
HEADER_DEST = ../include

SOURCES = $(wildcard *.c)
HEADERS = $(wildcard *.h)
OBJS = $(patsubst %.c,%.o,$(SOURCES))

libccan.a:$(OBJS)
	ar rcs $@ $?
	mv $@ ${LIB_DEST}
	cp ${HEADERS} ${HEADER_DEST}

%.o:%.c
	$(CC) -c $(CFLAGS) $(DEFINE) $(INCLUDE) $< -o $@	

clean:	
	rm -f $(OBJS)
	rm -f $(wildcard *.a)
