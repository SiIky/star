CC=musl-gcc
#CC=cc
EXEC=star
INPUT=main.c star.c
OBJS=$(INPUT:.c=.o)
DEST=~/bin

FLAGS=-static -Wall -Wextra -Werror
DFLAGS=$(FLAGS) -Og -g
CFLAGS=$(FLAGS) -O3

debug: $(INPUT)
	$(CC) $(DFLAGS) -c $(INPUT)
	$(CC) $(DFLAGS) $(OBJS) -o $(EXEC)

all: $(INPUT)
	$(CC) $(CFLAGS) -c $(INPUT)
	$(CC) $(CFLAGS) $(OBJS) -o $(EXEC)
	strip -s $(EXEC)

install: all
	install -m 0700 $(EXEC) -t $(DEST)

uninstall:
	rm -f $(DEST)/$(EXEC)

clean:
	rm -rf $(OBJS) $(EXEC)
