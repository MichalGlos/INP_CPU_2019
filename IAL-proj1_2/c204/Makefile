
PRJ=c204
#
C202PATH=../c202/
PROGS=$(PRJ)-test $(PRJ)-advanced-test
CC=gcc
CFLAGS=-Wall -std=c99 -pedantic -lm -I$(C202PATH)

all: $(PROGS)

$(PRJ)-test: $(PRJ).c $(PRJ)-test.c $(C202PATH)c202.c
	$(CC) $(CFLAGS) -o $@ $(PRJ).c $(PRJ)-test.c $(C202PATH)c202.c
$(PRJ)-advanced-test: $(PRJ).c $(PRJ)-advanced-test.c $(C202PATH)c202.c
	$(CC) $(CFLAGS) -o $@ $(PRJ).c $(PRJ)-advanced-test.c $(C202PATH)c202.c

run: all
	./$(PRJ)-test > $(PRJ)-test.out
	./$(PRJ)-advanced-test > $(PRJ)-advanced-test.out

diff: run
	diff $(PRJ)-test.out $(PRJ)-test.output
	diff $(PRJ)-advanced-test.out $(PRJ)-advanced-test.output

clean:
	rm -f *.o *.out $(PROGS)
#
