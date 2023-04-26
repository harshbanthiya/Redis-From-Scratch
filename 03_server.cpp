#include "03_redis.hpp"
/*
    one_request(int fd)
    Parses one request and replies 
    until something bad happens or client connection is gone
*/
static int32_t  one_request(int con_fd)
{
    // 4 bytes headers
    char    rbuff[4 + k_max_msg + 1]; 
    errno = 0;
    int32_t err = read_full(con_fd, rbuff, 4);

    if (err)
    {
        if (errno == 0)
            msg("EOF");
        else 
            msg("read() error");
        return err;
    }

    uint32_t len = 0;
    memcpy(&len, rbuff, 4); // Assume little endian
    if (len > k_max_msg)
    {
        msg("too long");
        return -1;
    }

    // Request body
    err = read_full(con_fd, &rbuff[4], len);
    if (err)
    {
        msg("read() error");
        return err;
    }

    // Do something
    rbuff[4 + len] = '\0';
    printf("client says: %s\n", &rbuff[4]);

    // Reply using the same protocol
    const char reply[] = "world";
    char wbuff[4 + sizeof(reply)];
    len = (uint32_t)strlen(reply);
    memcpy(wbuff, &len, 4);
    memcpy(&wbuff[4], reply, len);
    return write_all(con_fd, wbuff, 4 + len);
}


void do_something(int conn_fd) // Simply reads and writes 
{
	char buff[64] = {};
	ssize_t n = read(conn_fd, buff, sizeof(buff) - 1);

	if (n < 0)
    {
        msg("Read System Call: server side failed\n");
        return ;
    }

    
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
        // Only Serves one client connection at once 
        while (true)
        {
            int32_t err = one_request(conn_fd);
            if (err)
                break ;
        }
        do_something(conn_fd);
        close(conn_fd);
    }

}