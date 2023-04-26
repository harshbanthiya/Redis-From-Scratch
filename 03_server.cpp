#include <netinet/in.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <sys/select.h>
#include <unistd.h>

void err_exit(const char* str)
{
    write(2, str, strlen(str));
	exit(1) ;
}

void do_something(int conn_fd) // Simply reads and writes 
{
	char buff[64] = {};
	ssize_t n = read(conn_fd, buff, sizeof(buff) - 1);

	if (n < 0)
        err_exit("Read System Call: server side failed\n");

    
    char prompt[] = "Client says: ";
    write(1, prompt, strlen(prompt));
    write(1, buff, strlen(buff));

    char wbuff[] = "world";
	write(conn_fd, wbuff, sizeof(wbuff));
}

int main()
{
    int val = 1;
    int fd  = socket(AF_INET, SOCK_STREAM, 0); // AF_INET6 for ipv6 and SOCK_STREAM for TCP 

    setsockopt(fd, SOL_SOCKET, SO_REUSEADDR, &val, sizeof(val)); // Configuring various socket options we REUSE addresses here

    // Populate the socket struct
    struct sockaddr_in addr = {};
    addr.sin_family = AF_INET;
    addr.sin_port = htons(8080); // Or htons(atoi(argv[1]) if port is cmdline arg
    addr.sin_addr.s_addr = htonl(INADDR_LOOPBACK); // for sole access to/from localhost 127.0.0.1

    int rv = bind(fd, (const sockaddr*)&addr, sizeof(addr));
    if (rv < 0)
       err_exit("Error in Bind()\n"); // error in binding 

    rv = listen(fd, SOMAXCONN); // SOMAXCON - max connections 
    if (rv < 0)
        err_exit("Error in Listen() \n"); // error in listening 

    while (true)
    {
        // Accept clients
        struct sockaddr_in client_addr = {};
        socklen_t socklen = sizeof(client_addr);
        
        int conn_fd = accept(fd, (struct sockaddr *)&client_addr, &socklen);
        if (conn_fd < 0)
            continue ; // error loop again
        do_something(conn_fd);
        close(conn_fd);
    }

}