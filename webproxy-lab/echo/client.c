#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netdb.h>

#define BUFFER_SIZE 4096

static void usage(const char *progname)
{
    fprintf(stderr, "Usage: %s <host> <port>\n", progname);
}

static int open_clientfd(const char *hostname, const char *port)
{
    struct addrinfo hints;
    struct addrinfo *listp;
    struct addrinfo *p;
    int clientfd;
    int rc;

    memset(&hints, 0, sizeof(hints));
    hints.ai_socktype = SOCK_STREAM;
    hints.ai_flags = AI_ADDRCONFIG | AI_NUMERICSERV;

    if ((rc = getaddrinfo(hostname, port, &hints, &listp)) != 0) {
        fprintf(stderr, "getaddrinfo error: %s\n", gai_strerror(rc));
        return -1;
    }

    for (p = listp; p != NULL; p = p->ai_next) {
        clientfd = socket(p->ai_family, p->ai_socktype, p->ai_protocol);
        if (clientfd < 0) {
            continue;
        }

        if (connect(clientfd, p->ai_addr, p->ai_addrlen) == 0) {
            break;
        }

        close(clientfd);
    }

    freeaddrinfo(listp);

    if (p == NULL) {
        return -1;
    }

    return clientfd;
}

int main(int argc, char **argv)
{
    int clientfd;
    char sendbuf[BUFFER_SIZE];
    char recvbuf[BUFFER_SIZE];

    if (argc != 3) {
        usage(argv[0]);
        return 1;
    }

    clientfd = open_clientfd(argv[1], argv[2]);
    if (clientfd < 0) {
        fprintf(stderr, "Failed to connect to %s:%s\n", argv[1], argv[2]);
        return 1;
    }

    printf("Connected to %s:%s\n", argv[1], argv[2]);
    printf("Type a message and press Enter. Press Ctrl+D to quit.\n");

    while (fgets(sendbuf, sizeof(sendbuf), stdin) != NULL) {
        size_t len = strlen(sendbuf);
        size_t total_sent = 0;

        while (total_sent < len) {
            ssize_t nwritten = write(clientfd, sendbuf + total_sent, len - total_sent);
            if (nwritten <= 0) {
                fprintf(stderr, "Write error\n");
                close(clientfd);
                return 1;
            }
            total_sent += (size_t)nwritten;
        }

        ssize_t nread = read(clientfd, recvbuf, sizeof(recvbuf) - 1);
        if (nread <= 0) {
            fprintf(stderr, "Server closed connection\n");
            break;
        }

        recvbuf[nread] = '\0';
        printf("echoed: %s", recvbuf);
    }

    close(clientfd);
    return 0;
}
