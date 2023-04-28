server: server.cpp header_and_utils.hpp
	g++ -std=c++11 -Wall -Wextra -O2 -g server.cpp -o server

client: client.cpp header_and_utils.hpp
	g++ -std=c++11 -Wall -Wextra -O2 -g client.cpp -o client

clean:
	rm -rf server client 
