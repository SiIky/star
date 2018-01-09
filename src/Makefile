CC=musl-gcc
#CC=cc
EXEC=star
INPUT=main.c star.c
OBJS=$(INPUT:.c=.o)
DEST=~/bin

# _XOPEN_SOURCE for `strdup()`
# -I/usr/local/include/ for <utils/ifjmp.h>
CFLAGS=\
      -D_XOPEN_SOURCE=500 \
      -std=c11 \
      -static \
      -Wpedantic \
      -Wall \
      -Wextra \
      -Werror \
      -I/usr/local/include/

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
