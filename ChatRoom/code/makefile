# Make file for Server
# WRITTEN BY: CS513 team 2
# 

all: server

server: server.o
	gcc -Wall server.o -o server -lpthread

server.o: server.h srvgen.h srvstr.h server.c
	gcc -Wall -c server.c -lpthread

clean:
	rm -f server.o server
