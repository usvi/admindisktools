CC = gcc
FILE_OFFSET_FLAGS = -D_FILE_OFFSET_BITS=64

all: diskcont

adt_shared.o: adt_shared.h adt_shared.c
	$(CC) $(FILE_OFFSET_FLAGS) -Wall -c adt_shared.c

diskcont: diskcont.c adt_shared.o
	$(CC) $(FILE_OFFSET_FLAGS) -Wall -o ../bin/diskcont diskcont.c adt_shared.o

clean:
	@rm -f ../bin/diskcont
	@rm -f *.o
