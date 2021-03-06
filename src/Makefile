CC=musl-gcc
#CC=cc
EXEC=star
HEADERS=\
	star.h \
	stream.h

INPUT=\
      main.c \
      star.c \
      stream.c

OBJS=$(INPUT:.c=.o)
DEST=~/bin

# for <utils/ifjmp.h>
INCLUDE=\
	-I/usr/local/include/

# _XOPEN_SOURCE for `strdup()`
CFLAGS=\
       -D_XOPEN_SOURCE=500 \
       -Wall               \
       -Wconversion        \
       -Wextra             \
       -Wpedantic          \
       -static             \
       -std=c11            \
       $(INCLUDE)

debug: $(INPUT)
	$(CC) $(CFLAGS) -Og -g -c $(INPUT)
	$(CC) $(CFLAGS) -Og -g $(OBJS) -o $(EXEC)

all: $(INPUT)
	$(CC) $(CFLAGS) -O3 -c $(INPUT)
	$(CC) $(CFLAGS) -O3 $(OBJS) -o $(EXEC)
	strip -s $(EXEC)

install: all
	install -m 0700 $(EXEC) -t $(DEST)

uninstall:
	rm -f $(DEST)/$(EXEC)

clean:
	rm -rf $(OBJS) $(EXEC)

check: $(INPUT) $(HEADERS)
	cppcheck --std=c11 -f $(INCLUDE) --language=c --enable=warning $(INPUT) $(HEADERS)

tags:
	ctags --language-force=c --tag-relative=yes -f ../.tags $(INPUT) $(HEADERS)
