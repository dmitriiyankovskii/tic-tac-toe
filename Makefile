CC = gcc

all: client server

client: client.o
	$(CC) -o $@ $< 

server: server.o
	$(CC) -o $@ $< 


clean:
	rm -f *.o
	rm -f client
	rm -f server

%.o:%.c
	$(CC) -c -o $@ $^ 

.PHONY: clean
