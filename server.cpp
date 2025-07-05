#include <sys/socket.h>
#include <netinet/in.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <cassert>
#include <cerrno>

void die(const char *msg) {
    perror(msg);
    exit(1);
}

static int32_t read_full(int fd, char *buf, size_t n) {
    while (n > 0) {
        ssize_t rv = read(fd, buf, n);
        if (rv <= 0) {
            return -1; // error, or unexpected EOF
        }
        assert((size_t)rv <= n);
        n -= (size_t)rv;
        buf += rv; // move write pointer as to not overwrite 
    }
    return 0;
}

static int32_t write_all(int fd, const char *buf, size_t n) {
    while (n > 0) {
        ssize_t rv = write(fd, buf, n);
        if (rv <= 0) {
            return -1;
        }
        assert((size_t)rv <= n);
        n -= (size_t)rv; 
        buf += rv;
    }
    return 0;
}

const size_t k_max_msg = 4096;

static int32_t one_request(int connfd) {
    // 4 bytes header
    char rbuf[4 + k_max_msg];
    errno = 0;
    int32_t err = read_full(connfd, rbuf, 4);
    if (err) {
        die(errno == 0 ? "EOF" : "read() error");
        return err;
    }
    uint32_t len = 0;
    memcpy(&len, rbuf, 4);
    if (len > k_max_msg) {
        die("too long");
        return -1;
    }
    // request body
    err = read_full(connfd, &rbuf[4], len);
    if (err) {
        die("read() error");
        return err;
    }
    // do something
    printf("client says: %.*s\n", len, &rbuf[4]);
    // reply using same protocol
    const char reply[] = "world"; // reply = payload + 1 bytes
    char wbuf[4 + sizeof(reply)]; // buffer = reply + 4 bytes
    len = (uint32_t)strlen(reply); // 4 byte header = payload length
    memcpy(wbuf, &len, 4); // copy length of payload into first 4 bytes of buffer
    memcpy(&wbuf[4], reply, len); // copy payload contents into rest of buffer
    return write_all(connfd, wbuf, 4 + len);
}

int main() {
    int fd = socket(AF_INET, SOCK_STREAM, 0);

    int val = 1;
    setsockopt(fd, SOL_SOCKET, SO_REUSEADDR, &val, sizeof(val));

    struct sockaddr_in addr = {};
    addr.sin_family = AF_INET;
    addr.sin_port = htons(1234);
    addr.sin_addr.s_addr = htonl(0);
    int rv = bind(fd, (const struct sockaddr *)&addr, sizeof(addr));
    if (rv) { die("bind()"); }

    rv = listen(fd, SOMAXCONN);
    if (rv) { die("listen()"); }

    while (true) {
        struct sockaddr_in client_addr = {};
        socklen_t addrlen = sizeof(client_addr);
        int connfd = accept(fd, (struct sockaddr *)&client_addr, &addrlen);
        if (connfd < 0) {
            continue;
        }
        while (true) {
            int32_t err = one_request(connfd);
            if (err) {
                break;
            }
        }
        close(connfd);
    }
}