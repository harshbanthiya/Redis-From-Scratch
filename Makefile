CC=g++
CFLAGS=-std=c++11 -Wall -Wextra -O2 -g

server: server.cpp hashtable.cpp hashtable.h
	g++ -std=c++11 -Wall -Wextra -O2 -g server.cpp hashtable.cpp -o server

client: client.cpp
	$(CC) $(CFLAGS) $^ -o $@

clean:
	rm -rf server client