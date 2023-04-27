#include <netinet/in.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <sys/select.h>
#include <sys/errno.h>
#include <unistd.h>
#include <fcntl.h>
#include <assert.h>

const size_t    k_max_msg = 4096;

enum {
    STATE_REQ = 0, // Reading Requests
    STATE_RES = 1, // Sending Responses
    STATE_END = 2,
};

// IO operations are deferred so lets keep em read and write buffers separate
typedef struct      s_connection
{
    int     fd      = -1;
    uint32_t state  = 0; 

    // Buffer for reading
    size_t  rbuff_size = 0;
    uint8_t rbuff[4 + k_max_msg];
    
    // Buffer for writing
    size_t  wbuff_size = 0;
    size_t  wbuff_sent = 0;
    uint8_t wbuff[4 + k_max_msg];
}                   conn;


void err_exit(const char* str)
{
    write(2, str, strlen(str));
	exit(1) ;
}

void msg(const char* str)
{
    write(1, str, strlen(str));
}

/*
    read system call returns data available in the kernel or blocks if there is none
    read_full reads from the Kernel until it got exactly n bytes 
*/
static int32_t  read_full(int fd, char *buff, size_t n)
{
    while (n > 0)
    {
        ssize_t rv = read(fd, buff, n);
        if (rv <= 0)
            return -1;
        assert((size_t) rv <= n);
        n -= (size_t)rv;
        buff += rv;
    }
    return (0);
}
/*
    write() system call can return successfully with partial data written if kernel buffer is full
    write_all keeps trying when write returns fewers bytes than we need
*/

static int32_t write_all(int fd, const char *buff, size_t n)
{
    while (n > 0)
    {
        ssize_t rv = write(fd, buff, n);
        if (rv <= 0)
            return -1;
        assert((size_t) rv <= n);
        n -= (size_t)rv;
        buff += rv;
    }
    return 0;
}

static void fd_set_nb(int fd)
{
    errno = 0;

    int flags = fcntl(fd, F_GETFL, 0);
    if (errno)
    {
        err_exit("fcntl error\n");
        return ;
    }

    flags |= O_NONBLOCK;

    errno = 0;
    (void)fcntl(fd, F_SETFL, flags);
    if (errno)
        err_exit("fnctl error\n");
}