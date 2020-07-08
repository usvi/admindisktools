CC = gcc
FILE_OFFSET_FLAGS = -D_FILE_OFFSET_BITS=64
LINK_PTHREAD = -pthread

all: ../bin/diskcont ../bin/diskinfo ../bin/raidkill

adt_shared.o: adt_shared.h adt_shared.c
	$(CC) $(FILE_OFFSET_FLAGS) -Wall -c adt_shared.c

../bin/diskcont: diskcont.c adt_shared.o
	$(CC) -Wall $(FILE_OFFSET_FLAGS) $(LINK_PTHREAD) adt_shared.o diskcont.c -o ../bin/diskcont

../bin/diskinfo: diskinfo.c adt_shared.o
	$(CC) -Wall $(FILE_OFFSET_FLAGS) adt_shared.o diskinfo.c -o ../bin/diskinfo

../bin/raidkill: raidkill.c adt_shared.o
	$(CC) -Wall $(FILE_OFFSET_FLAGS) adt_shared.o raidkill.c -o ../bin/raidkill

clean:
	@rm -f ../bin/diskcont
	@rm -f ../bin/diskinfo
	@rm -f ../bin/raidkill
	@rm -f *.o
