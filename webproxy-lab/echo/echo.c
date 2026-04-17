#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netdb.h>
#include <arpa/inet.h>

#define BACKLOG 10
#define BUFFER_SIZE 4096

static void usage(const char *progname)
{
    fprintf(stderr, "Usage: %s <port>\n", progname);
}

static int open_listenfd(const char *port)
{
    struct addrinfo hints;
    struct addrinfo *listp;
    struct addrinfo *p;
    int listenfd;
    int optval = 1;
    int rc;

    memset(&hints, 0, sizeof(hints));
    hints.ai_socktype = SOCK_STREAM;
    hints.ai_flags = AI_PASSIVE | AI_ADDRCONFIG | AI_NUMERICSERV;

    if ((rc = getaddrinfo(NULL, port, &hints, &listp)) != 0) {
        fprintf(stderr, "getaddrinfo error: %s\n", gai_strerror(rc));
        return -1;
    }

    for (p = listp; p != NULL; p = p->ai_next) {
        listenfd = socket(p->ai_family, p->ai_socktype, p->ai_protocol);
        if (listenfd < 0) {
            continue;
        }

        setsockopt(listenfd, SOL_SOCKET, SO_REUSEADDR, (const void *)&optval, sizeof(int));

        if (bind(listenfd, p->ai_addr, p->ai_addrlen) == 0) {
            break;
        }

        close(listenfd);
    }

    freeaddrinfo(listp);

    if (p == NULL) {
        return -1;
    }

    if (listen(listenfd, BACKLOG) < 0) {
        close(listenfd);
        return -1;
    }

    return listenfd;
}

static void echo_client(int connfd)
{
    char buffer[BUFFER_SIZE];
    ssize_t nread;

    while ((nread = read(connfd, buffer, sizeof(buffer))) > 0) {
        ssize_t total_written = 0;

        while (total_written < nread) {
            ssize_t nwritten = write(connfd, buffer + total_written, (size_t)(nread - total_written));
            if (nwritten <= 0) {
                return;
            }
            total_written += nwritten;
        }
    }
}

void echo(int connfd);

int main(int argc, char **argv) // agrc : 명령줄 인자의 개수, argv : 명령줄 인자 문자열 배열 -> argv[0] : "./echoserver", argv[1] = "8000"(서버가 기다릴 포트 번호)
{
    int listenfd, connfd; // //듣기 식별자, 연결 식별자
    socklen_t clientlen; // 클라이언트 주소 구조체 길이 저장 -> accept에서 클라이언트 주소를 받아올 때 -> 주소를 저장할 메모리공간, 그 공간의 크기 알아야 함

    // 클라이언트 주소를 담아둘 큰 그릇
    // IPv4일 수도 있고, IPv6일 수도 있어서 둘 다 안전하게 담을 수 있는 넉넉한 공통 구조체를 쓰는 것
    struct sockaddr_storage clientaddr;

    // host : 클라이언트 host 정보가 들어갈 문자열, service : 클라이어느트이 service 정보가 들어갈 문자열
    // 나중에 getnameinfo가 주소 구조체를 사람이 읽을 수 있는 문자열로 바꿔서 여기에 넣어준다
    char host[MAXLINE], service[MAXLINE];

    if(argc != 2)
    {
        fprintf(stderr, "usage: %s <port>\n", argv[0]);
        exit(0);
    }

    // 포트번호 가지고 서버용 listening socket을 만든다
    // ex) 터미널에서 ./echoserver 8000 입력 
    //
    listenfd = Open_listenfd(argv[1]);

    // 클라이언트 한 명 처리
    // 또 다음 사람 처리
    // 또 다음 사람 처리 반복
    while(1)
    {   
        //
        clientlen = sizeof(struct sockaddr_storage);
    }
}
