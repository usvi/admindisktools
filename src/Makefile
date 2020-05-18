CC = gcc
FILE_OFFSET_FLAGS = -D_FILE_OFFSET_BITS=64

all: ../bin/diskcont ../bin/diskinfo

adt_shared.o: adt_shared.h adt_shared.c
	$(CC) $(FILE_OFFSET_FLAGS) -Wall -c adt_shared.c

../bin/diskcont: diskcont.c adt_shared.o
	$(CC) -Wall $(FILE_OFFSET_FLAGS) adt_shared.o diskcont.c -o ../bin/diskcont

../bin/diskinfo: diskinfo.c adt_shared.o
	$(CC) -Wall $(FILE_OFFSET_FLAGS) adt_shared.o diskinfo.c -o ../bin/diskinfo

clean:
	@rm -f ../bin/diskcont
	@rm -f ../bin/diskinfo
	@rm -f *.o
