CC=gcc

CFLAGS = -Wall -O3

INCLUDE = -I. -I../include
LIB += -Wl,-Bstatic -L../lib -lxxhash -lccan 
LIB += -Wl,-Bdynamic -L../lib -lcatboostmodel -lm


SOURCES = $(wildcard *.c)
HEADERS = $(wildcard *.h)

main:${SOURCES} ${HEADERS} Makefile
	$(CC) $(CFLAGS) -o $@ $(SOURCES) $(INCLUDE) $(LIB)
	
clean:
	rm -f ./main
