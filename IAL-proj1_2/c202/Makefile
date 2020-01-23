
PRJ=c202
#
PROGS=$(PRJ)-test $(PRJ)-advanced-test
CC=gcc
CFLAGS=-Wall -std=c99 -pedantic -lm

all: $(PROGS)

$(PRJ)-test: $(PRJ).c $(PRJ)-test.c
	$(CC) $(CFLAGS) -o $@ $(PRJ).c $(PRJ)-test.c
$(PRJ)-advanced-test: $(PRJ).c $(PRJ)-advanced-test.c
	$(CC) $(CFLAGS) -o $@ $(PRJ).c $(PRJ)-advanced-test.c

run: all
	./$(PRJ)-test > $(PRJ)-test.out
	./$(PRJ)-advanced-test > $(PRJ)-advanced-test.out

diff: run
	diff $(PRJ)-test.out $(PRJ)-test.output
	diff $(PRJ)-advanced-test.out $(PRJ)-advanced-test.output

clean:
	rm -f *.o *.out $(PROGS)
#