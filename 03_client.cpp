#include "03_redis.hpp"

static int32_t query(int fd, const char *text)
{
    uint32_t len = (uint32_t) strlen(text);

    if (len > k_max_msg)
        return (-1);
    
    char wbuf[4 + k_max_msg];
    memcpy(wbuf, &len, 4);
    memcpy(&wbuf[4], &len, 4);
    
    if (int32_t err = write_all(fd, wbuf, 4 + len))
        return err;
    
    // 4 byte header 
    char rbuff[4 + k_max_msg + 1];
    errno = 0;
    int32_t err = read_full(fd, rbuff, 4);
    if (err)
    {
        if (errno == 0)
            msg("EOF");
        else 
            msg("read() error");
        return err;
    }

    memcpy(&len, rbuff, 4); 
    if (len > k_max_msg)
    {
        msg("too long");
        return -1;
    }

    // reply body
    err = read_full(fd, &rbuff[4], len);
    if (err)
    {
        msg("read() error");
        return err;
    }

    // Do something 
    rbuff[4 + len] = '\0';
    printf("server says: %s\n", &rbuff[4]);
    return (0);
}

int main()
{
    int fd = socket(AF_INET, SOCK_STREAM, 0);
	  if (fd < 0) 
			err_exit("Socket() client \n");  // Socket error

    struct sockaddr_in addr = {};
    addr.sin_family = AF_INET;
    addr.sin_port = htons(8080);
    addr.sin_addr.s_addr = htonl(INADDR_LOOPBACK);  // 127.0.0.1
    
		int rv = connect(fd, (const struct sockaddr *)&addr, sizeof(addr));
    if (rv < 0)
			err_exit("Connect() error\n"); // Connection error

    char msg[] = "hello";
    write(fd, msg, strlen(msg));

    // int32_t err = query(fd, "hello1");
    // if (err) {
    //     return -1;
    // }
    // err = query(fd, "hello2");
    // if (err) {
    //    return -1;
    // }
    // err = query(fd, "hello3");
    // if (err) {
    //   return -1;
    // }

    char rbuf[64] = {};
	ssize_t n = read(fd, rbuf, sizeof(rbuf) - 1);
    if (n < 0) {
       err_exit("Read() client()\n"); // read error 
    }
    printf("server says: %s\n", rbuf);
    close(fd);
}