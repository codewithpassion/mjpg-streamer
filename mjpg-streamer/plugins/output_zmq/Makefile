###############################################################
#
# Purpose: Makefile for "M-JPEG Streamer"
# Author.: Tom Stoeveken (TST)
# Version: 0.3
# License: GPL
#
###############################################################

CC = gcc -std=c11

OTHER_HEADERS = ../../mjpg_streamer.h ../../utils.h ../output.h ../input.h

CFLAGS += -O2 -DLINUX -D_GNU_SOURCE -Wall -shared -fPIC
#CFLAGS += -DDEBUG
LFLAGS += -lpthread -ldl -lzmq

all: output_zmq.so

clean:
	rm -f *.a *.o core *~ *.so *.lo

output_zmq.so: $(OTHER_HEADERS) output_zmq.c
	$(CC) $(CFLAGS) -o $@ output_zmq.c $(LFLAGS) 
