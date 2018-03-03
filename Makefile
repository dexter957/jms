CC = gcc
HDRS = jms_header.h jobsCatalogue.h poolList.h
SRCS = jms_console.c jms_coord.c pool.c jobsCatalogue.c poolList.c 
OBJS = $(SRCS: .c=.o)
CONSOBJS = jms_console.o
COORDOBJS = jms_coord.o poolList.o
POOLOBJS = pool.o jobsCatalogue.o
CONSOLE = ./jms_console
COORD = ./jms_coord
POOL = ./pool

all: $(CONSOLE) $(COORD) $(POOL)

$(CONSOLE) : $(CONSOBJS)
	$(CC) -o $@ $(CONSOBJS)

$(COORD) : $(COORDOBJS)
	$(CC) -o $@ $(COORDOBJS)

$(POOL) : $(POOLOBJS)
	$(CC) -o $@ $(POOLOBJS)


.PHONY : clean
clean : 
	rm *.o	