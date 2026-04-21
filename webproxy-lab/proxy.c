#include <stdio.h>
#include "csapp.h"

/* Recommended max cache and object sizes */
#define MAX_CACHE_SIZE 1049000
#define MAX_OBJECT_SIZE 102400

/* You won't lose style points for including this long line in your code */
static const char *user_agent_hdr =
    "User-Agent: Mozilla/5.0 (X11; Linux x86_64; rv:10.0.3) Gecko/20120305 "
    "Firefox/10.0.3\r\n";


void doit(int fd);

/*
 * doit
 * - 브라우저가 보낸 요청 라인을 읽는다.
 * - method / uri / version 을 파싱한다.
 * - 우선 GET만 처리하고, 다른 메서드는 에러로 돌려보낸다.
 * - URI에서 hostname, port, path 를 분리한다.
 * - end server 와 연결한 뒤 새 HTTP 요청을 보낸다.
 * - end server 의 응답을 읽어서 클라이언트에게 그대로 relay 한다.
 */

/*
 * parse_uri
 * - 예: http://localhost:8000/home.html
 * - hostname = localhost
 * - port = 8000
 * - path = /home.html
 * - 포트가 없으면 기본값 80을 사용한다.
 */
int parse_uri(char *uri, char *hostname, char *path, char *port);

/*
 * build_http_header
 * - proxy가 end server로 보낼 새 요청 헤더를 만든다.
 * - 요청줄은 "GET /path HTTP/1.0" 형태로 만든다.
 * - Host, User-Agent, Connection, Proxy-Connection 헤더를 적절히 구성한다.
 * - 브라우저가 보낸 나머지 헤더 중 필요한 것은 함께 붙인다.
 */
void build_http_header(char *http_header, char *hostname, char *path,
                       char *port, rio_t *client_rio);

/*
 * clienterror
 * - 잘못된 메서드, 잘못된 URI, 서버 연결 실패 같은 상황에서
 *   클라이언트에게 HTTP 에러 응답을 보낸다.
 */
void clienterror(int fd, char *cause, char *errnum, char *shortmsg,
                 char *longmsg);


/*
 * main
 * - 프록시가 사용할 listen socket을 연다.
 * - 클라이언트 연결을 반복해서 받고, 각 연결을 doit()으로 넘긴다.
 * - 처음에는 iterative 방식으로 구현하고, 이후 필요하면 thread를 붙이면 된다.
 */
int main(int argc, char **argv)
{
  int listenfd, connfd; // listen소켓 번호, 클라이언트 소켓 번호
  char hostname[MAXLINE], port[MAXLINE]; // hostname : 어느 서버인지, port : 그 서버의 어느 서비스인지 
  socklen_t clientlen; // 클라이언트 주소 구조체의 크기 알려주기 위해 쓴다
  struct sockaddr_storage clientaddr; //클라이언트 주소 정보 받아올 것

  if(argc != 2)
  {
    fprintf(stderr, "usage: %s <port>\n", argv[0]);
    exit(1);
  }

  listenfd = Open_listenfd(argv[1]); // 프록시 서버가 해당 포트에서 클라 연결을 기다릴 수 있게 여는 것

  while(1)
  {
    clientlen = sizeof(clientaddr); // 클라이언트 주소 구조체 크기
    connfd = Accept(listenfd, (SA *)&clientaddr, &clientlen); // 클라가 connect한 신호를 받는다
    Getnameinfo((SA*)&clientaddr, clientlen, hostname, MAXLINE, port, MAXLINE, 0); // 문자열로 바꿔준다
    printf("Accepted connection from (%s, %s)\n", hostname, port);

    doit(connfd);
    Close(connfd);
  }


}
