#include "03_redis.hpp"
#include <vector>
#include <poll.h>

static bool try_flush_buffer(conn *Conn)
{
    ssize_t rv = 0;

    do {
        size_t remain =  Conn->wbuff_size - Conn->wbuff_sent ;
        rv = write(Conn->fd, &Conn->wbuff[Conn->wbuff_sent], remain);
    }while(rv < 0 && errno == EINTR);
    
    if (rv < 0 && errno == EAGAIN)
        return false;
    
    if (rv < 0)
    {
        msg("write() error");
        Conn->state = STATE_END;
        return false;
    }

    Conn->wbuff_sent += (size_t)rv;
    assert(Conn->wbuff_sent <= Conn->wbuff_size);
    if (Conn->wbuff_sent == Conn->wbuff_size)
    {
        // response fully sent, change state back
        Conn->state = STATE_REQ;
        Conn->wbuff_sent = 0;
        Conn->wbuff_size = 0;
        return false;
    }
    return true;
}

static void state_res(conn * Conn)
{
    while(try_flush_buffer(Conn)) {}
}

static bool try_one_request(conn* Conn)
{
    // try to parse request from the buffer 
    if (Conn->rbuff_size < 4)
        return false; // Not enough data in buffer retry
    uint32_t len = 0;
    memcpy(&len, &Conn->rbuff[0], 4);
    if (len > k_max_msg)
    {
        msg("too long");
        Conn->state = STATE_END;
        return false;
    }
    if (4 + len > Conn->rbuff_size)
        return false; 
    // Got one req, do something with it
    printf("client says: %.*s\n", len, &Conn->rbuff[4]);

    // Generating Echoing Response
    memcpy(&Conn->wbuff[0], &len, 4);
    memcpy(&Conn->wbuff[4], &Conn->rbuff[4], len);
    Conn->wbuff_size = 4 + len;

    // Remove request from buffer 
    size_t remain = Conn->rbuff_size - 4 - len;
    if (remain)
        memmove(Conn->rbuff, &Conn->rbuff[4 + len], remain);
    Conn->rbuff_size = remain;

    // Change State
    Conn->state = STATE_RES;
    state_res(Conn);

    // Continue outer loop if request was fully processed 
    return (Conn->state == STATE_REQ);
}

static bool try_fill_buffer(conn* Conn)
{
    assert(Conn->rbuff_size < sizeof(Conn->rbuff));
    ssize_t  rv = 0;

    do {
        size_t cap = sizeof(Conn->rbuff) - Conn->rbuff_size;
        rv = read(Conn->fd, &Conn->rbuff[Conn->rbuff_size], cap);
    }while(rv < 0 && errno == EINTR);

    if (rv < 0 && errno == EAGAIN)
        return false;
    
    if (rv < 0)
    {
        msg("read() error");
        Conn->state = STATE_END;
        return false;
    }
    if (rv == 0)
    {
        if (Conn->rbuff_size > 0)
            msg("Unexpected EOF\n");
        else 
            msg("EOF");
        Conn->state = STATE_END;
        return false;
    }
    Conn->rbuff_size += (size_t)rv;
    assert(Conn->rbuff_size  <= sizeof(Conn->rbuff) - Conn->rbuff_size);

    while(try_one_request(Conn)) {}
    return (Conn->state == STATE_REQ);
}

static void state_req(conn *Conn)
{
    while(try_fill_buffer(Conn))
    {}
}


static void conn_put(std::vector<conn *>&fd2conn, conn* Conn)
{
    if (fd2conn.size() <= (size_t)Conn->fd)
        fd2conn.resize(Conn->fd + 1);
    fd2conn[Conn->fd] = Conn;
}

static int32_t accept_new_conn(std::vector<conn *>&fd2conn, int fd)
{
    // Accept
    struct sockaddr_in client_addr = {};
    socklen_t socklen = sizeof(client_addr);

    int connfd = accept(fd, (struct sockaddr *)&client_addr, &socklen);
    if (connfd < 0)
    {
        msg("accept() error");
        return (-1);
    }
    
    // Set new connection fd to non blocking 
    fd_set_nb(connfd);

    // Creating the struct Conn
    conn* Conn = (conn *)malloc(sizeof(conn));
    if (!Conn)
    {
        close(connfd);
        return (-1);
    }

    Conn->fd = connfd;
    Conn->state = STATE_REQ;
    Conn->rbuff_size = 0;
    Conn->wbuff_size = 0;
    Conn->wbuff_sent = 0;
    conn_put(fd2conn, Conn);
    return (0);
}   

// State Machine for Client 
static void connection_io(conn * Conn)
{
    if (Conn->state == STATE_REQ)
        state_req(Conn);
    else if (Conn->state == STATE_RES)
        state_res(Conn);
    else 
        assert(0);
}

int main()
{
    // int val = 1;
    int fd  = socket(AF_INET, SOCK_STREAM, 0); // AF_INET6 for ipv6 and SOCK_STREAM for TCP 
    if (fd < 0)
        err_exit("socket() error\n");
    // setsockopt(fd, SOL_SOCKET, SO_REUSEADDR, &val, sizeof(val)); // Configuring various socket options we REUSE addresses here

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

    // map of all client connections 
    std::vector<conn *> fd2conn;

    // Set the listen fd to non blocking 
    fd_set_nb(fd);

    // Event loop
    std::vector<struct pollfd> poll_args;
    while (true)
    {
        poll_args.clear();
        struct pollfd pfd = {fd, POLLIN, 0};

        poll_args.push_back(pfd);

        // Connection fds
        for (conn *Conn : fd2conn)
        {
            if (!Conn)
                continue;
            
            struct pollfd pfd = {};
            pfd.fd = Conn->fd;
            pfd.events = (Conn->state == STATE_REQ) ? POLLIN : POLLOUT;
            pfd.events = pfd.events | POLLERR;
            poll_args.push_back(pfd);
        }

        // Poll Active Fds 
        int rv = poll(poll_args.data(), (nfds_t)poll_args.size(), 1000);
        if (rv < 0)
            err_exit("Poll() \n");
        
        for (size_t i = 1; i < poll_args.size(); ++i)
        {
            if (poll_args[i].revents)
            {
                conn *Conn = fd2conn[poll_args[i].fd];
                connection_io(Conn);

                if (Conn->state == STATE_END) // client closed or something bad happened destroy this connection
                {
                    fd2conn[Conn->fd] = NULL;
                    (void) close(Conn->fd);
                    free(Conn);
                }
            }
        }
        if (poll_args[0].revents)
            (void)accept_new_conn(fd2conn, fd);
    }
    return (0);
}