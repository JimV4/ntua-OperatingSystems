CC = gcc

# CAUTION: Always use '-pthread' when compiling POSIX threads-based
# applications, instead of linking with "-lpthread" directly.
CFLAGS = -Wall -O2 -pthread
LIBS =

all: pthread-test simplesync-mutex simplesync-atomic kgarten kgartenTest kgartenTest2

## Pthread test
pthread-test: pthread-test.o
	$(CC) $(CFLAGS) -o pthread-test pthread-test.o $(LIBS)

pthread-test.o: pthread-test.c
	$(CC) $(CFLAGS) -c -o pthread-test.o pthread-test.c

## Simple sync (two versions)
simplesync-mutex: simplesync-mutex.o
	$(CC) $(CFLAGS) -o simplesync-mutex simplesync-mutex.o $(LIBS)

simplesync-atomic: simplesync-atomic.o
	$(CC) $(CFLAGS) -o simplesync-atomic simplesync-atomic.o $(LIBS)

simplesync-mutex.o: simplesync.c
	$(CC) $(CFLAGS) -DSYNC_MUTEX -c -o simplesync-mutex.o simplesync.c

simplesync-atomic.o: simplesync.c
	$(CC) $(CFLAGS) -DSYNC_ATOMIC -c -o simplesync-atomic.o simplesync.c

## Kindergarten
kgarten: kgarten.o
	$(CC) $(CFLAGS) -o kgarten kgarten.o $(LIBS)

kgarten.o: kgarten.c
	$(CC) $(CFLAGS) -c -o kgarten.o kgarten.c

## Kindergarten Test
kgartenTest: kgartenTest.o
	$(CC) $(CFLAGS) -o kgartenTest kgartenTest.o $(LIBS)

kgartenTest.o: kgartenTest.c
	$(CC) $(CFLAGS) -c -o kgartenTest.o kgartenTest.c

## Kindergarten Test 2
kgartenTest2: kgartenTest2.o
	$(CC) $(CFLAGS) -o kgartenTest2 kgartenTest2.o $(LIBS)

kgartenTest2.o: kgartenTest2.c
	$(CC) $(CFLAGS) -c -o kgartenTest2.o kgartenTest2.c
## Mandel
#mandel: mandel-lib.o mandel.o
#	$(CC) $(CFLAGS) -o mandel mandel-lib.o mandel.o $(LIBS)

#mandel-lib.o: mandel-lib.h mandel-lib.c
#	$(CC) $(CFLAGS) -c -o mandel-lib.o mandel-lib.c $(LIBS)

#mandel.o: mandel.c
#	$(CC) $(CFLAGS) -c -o mandel.o mandel.c $(LIBS)

clean:
	rm -f *.s *.o pthread-test simplesync-{atomic,mutex} kgarten kgartenTest kgartenTest2 #mandel
