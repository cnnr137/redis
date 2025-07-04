int fd = socket(AF_INET, SOCK_STREAM, 0);

int val = 1;
setsockopt(fd, SOL_SOCKET, SO_REUSEADDR, &val, sizeof(val));

struct sockaddr_in {
    uint16_t sin_family; // AF_INET
    uint16_t sin_port;   // port in big endian
    struct in_addr sin_addr; // IPv4
};

struct in_addr {
    uint32_t s_addr; // IPv4 in big endian
}

struct sockaddr_in addr = {};
addr.sin_family = AF_INET;
addr.sin_port = htons(1234); // port
addr.sin_addr.s_addr = htonl(0); // wildcard IP 0.0.0.0
int rv = bind(fd, (const struct sockaddr *)&addr, sizeof(addr));
if (rv) { die("bind()"); };

// listen 
rv = listen(fd, SOMAXCONN);
if (rv) { die("listen()") };

while (true) {
    //accept
    struct sockaddr_in client_addr = {};
    socklen_t addrlen = sizeof(client_addr);
    int connfd = accept(fd, (struct sockaddr *)&client_addr, &addrlen);
    if (connfd < 0) {
        continue; // error
    } 
    do_something(connfd);
    close(connfd);
}

static void do_something(int connfd) {
    char rbuf[64] = {};
    ssize_t n = read(connfd, rbuf, sizeof(rbuf) - 1);
    if (n < 0 ) {
        msg("read() error");
        return;
    }
    printf("client says: %s\n", rbuf);
    char wbuf[] = "world";
    write(connfd, wbuf, strlen(wbuf))
}