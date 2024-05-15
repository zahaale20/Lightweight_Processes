CC      = gcc
CFLAGS  = -Wall -g -I .
LD      = gcc
LDFLAGS = -Wall -g

PROGS       = snakes nums hungry
SNAKEOBJS   = randomsnakes.o
HUNGRYOBJS  = hungrysnakes.o
NUMOBJS     = numbersmain.o
OBJS        = $(SNAKEOBJS) $(HUNGRYOBJS) $(NUMOBJS)
SRCS        = randomsnakes.c numbersmain.c hungrysnakes.c

EXTRACLEAN  = core $(PROGS)

all: $(PROGS)

clean:
	rm -f $(OBJS) $(EXTRACLEAN) lwp.o magic64.o libLWP.a libLWP.so libsnakes.so


snakes: randomsnakes.o libLWP.so libsnakes.so
	#$(LD) $(LDFLAGS) -o snakes randomsnakes.o -L. -lncurses -lsnakes -lLWP -lPLN
	$(LD) $(LDFLAGS) -o snakes randomsnakes.o  libsnakes.a libLWP.a libPLN.a -lncurses
hungry: hungrysnakes.o libLWP.so libsnakes.so
	#$(LD) $(LDFLAGS) -o hungry hungrysnakes.o -L. -lncurses -lsnakes -lLWP -lPLN
	$(LD) $(LDFLAGS) -o hungry hungrysnakes.o libsnakes.a libLWP.a libPLN.a -lncurses


libsnakes.so:
	$(LD) -shared -o libsnakes.so libsnakes.a
nums: numbersmain.o libLWP.so
	$(LD) $(LDFLAGS) -o nums numbersmain.o libLWP.a libPLN.a -lncurses


libLWP.so: libLWP.a
	$(LD) -shared -o libLWP.so libLWP.a

hungrysnakes.o: lwp.h snakes.h

randomsnakes.o: lwp.h snakes.h

numbersmain.o: lwp.h

libLWP.a: 
	$(CC) -c lwp.c util.c rr.c thread_queue.c
	ar r libLWP.a lwp.o util.o rr.o thread_queue.o
	rm lwp.o util.o rr.o thread_queue.o

submission:
	tar -cf project2_submission.tar lwp.c lwp.h rr.c rr.h thread_queue.h thread_queue.c Makefile README
	gzip project2_submission.tar