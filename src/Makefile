CC = gcc

all: diskcont

diskcont: diskcont.c
	$(CC) -D_FILE_OFFSET_BITS=64 -Wall -o ../bin/diskcont diskcont.c

clean:
	@rm -f ../bin/diskcont
	@rm -f *.o
