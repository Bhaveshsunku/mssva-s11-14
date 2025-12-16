CC=clang
CFLAGS=-g -O0 -Wall -Iinclude

all: dataproc-agent

dataproc-agent:
	$(CC) $(CFLAGS) src/*.c -o dataproc-agent

asan:
	$(CC) $(CFLAGS) -fsanitize=address src/*.c -o dataproc-agent

clean:
	rm -f dataproc-agent
