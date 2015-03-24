
CC=gcc
CFLAGS=-g -Wall -O3
LDFLAGS=-g -lm

AO_LDFLAGS=`pkg-config --libs ao`

all: ax25beacon ax25frame

ax25beacon: ax25.o ax25beacon.o
	$(CC) -o ax25beacon ax25.o ax25beacon.o $(LDFLAGS) $(AO_LDFLAGS)

ax25frame: ax25.o ax25frame.o
	$(CC) -o ax25frame ax25.o ax25frame.o $(LDFLAGS) $(AO_LDFLAGS)

.c.o:
	$(CC) $(CFLAGS) -c $< -o $@

install:
	cp -f ax25beacon /usr/local/bin/
	cp -f ax25frame /usr/local/bin/

clean:
	rm -f *.o ax25beacon ax25frame

