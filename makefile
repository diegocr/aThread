CC	=	gcc
CFLAGS	=	-noixemul -m68060 -O2 -msmall-code -I. -W -Wall -Winline -DDEBUG
LIBS	=	-Wl,-Map,$@.map,--cref
#OBJS	=	athread.o debug.o main.o palloc.o usema.o
OBJS	=	thread.o test.o

all:	test
test:	$(OBJS)
	$(CC) $(CFLAGS) -s -o $@ $(OBJS) $(LIBS)
