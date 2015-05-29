# Make file for client
# WRITTEN BY: CS513 team 2
# 

all: client server


client: client.c DLL.c queue.c
	gcc client.c DLL.c -o client -lpthread

server: Server.c DLL.c queue.c
	gcc Server.c DLL.c -o server -lpthread

clean:
	rm -f server client
