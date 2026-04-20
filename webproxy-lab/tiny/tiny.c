/* $begin tinymain */
/*
 * tiny.c - A simple, iterative HTTP/1.0 Web server that uses the
 *     GET method to serve static and dynamic content.
 *
 * Updated 11/2019 droh
 *   - Fixed sprintf() aliasing issue in serve_static(), and clienterror().
 */
#include "csapp.h"

void doit(int fd);
void read_requesthdrs(rio_t *rp);
int parse_uri(char *uri, char *filename, char *cgiargs);
void serve_static(int fd, char *filename, int filesize);
void get_filetype(char *filename, char *filetype);
void serve_dynamic(int fd, char *filename, char *cgiargs);
void clienterror(int fd, char *cause, char *errnum, char *shortmsg,
                 char *longmsg);

// 요청 처리
void doit(inf fd)
{
  if(fd < 0)
  {
    printf("fd error");
  }

  struct stat sbuf;
  size_t n, isStatic;
  rio_t rio;
  char buf[MAXLINE];
  char method[MAXLINE], uri[MAXLINE], version[MAXLINE];

  // fileName : static이면 열어서 읽을 파일이름, dynamic이면 실행할 CGI 프로그램
  // cgiargs : static이면 빈 문자열, dynamic이면 물음표 뒤 인자들
  char filename[MAXLINE], char cgiargs[MAXLINE];


  //rio 구조체 초기화
  Rio_readinitb(&rio, fd);
  n = Rio_readlineb(&rio, buf, MAXLINE);

  // n이 0이면
  if(n == 0)
  {
    return;
  }
  else if(n < 0)
  {
    //읽기 에러
    printf("request line read failed");
    return;
  }

  //GET /index.html HTTP/1.0
  size_t parsedCount = sscanf(buf, "%s %s %s", method, uri, version); // 띄어쓰기 별로 끊어서 각각 저장

  if(parsedCount != 3)
  {
    clienterror(fd, buf, 400, "Bad Request", "Tiny could not parse request line");
    return;
  }

  if(strcmp(method, "GET") != 0)
  {
    clienterror(fd, buf, 501, "Not Implemented", "Tiny does not implement this method");
    return;
  }

  // 여기까지 오면  GET /index.html HTTP/1.0  이런 상태이다
  read_requesthdrs(&rio); // 헤더 무시


  // uri 해석해서, 실제 파일경로 넣고, cgi 인자 넣고
  isStatic = parse_uri(uri, filename, cgiargs); // 파서에서 uri보고 정적인지 동적인지 판단

  //파일이 없다/ 경로가 잘못됨 / 접근 불가
  if(stat(filename, sbuf) < 0)
  {
    clienterror(fd, filename, "404", "Not found", "Tiny couldn't find this file");
  }

  // 정적 콘텐츠 요청이면
  if(isStatic)
  {
    serve_static(fd, filename, sbuf.st_size);
  }

  // 동적 콘텐츠 요청이면
  else
  {
    serve_dynamic(fd, filename, sbuf.st_size);
  }


}

// doit에서 쓰는 rio를 넘겨받는다
// 남은 헤더 줄 다 읽어버려서 무시
void read_requesthdrs(rio_t *rp)
{
  char buf[MAXLINE];

  //한 줄씩 읽는다
  
  while(1)
  {
    // 한줄 읽는다
    size_t n = Rio_readlineb(rp, buf, MAXLINE);


    if(n == 0)
    {
      break;
    }

    // 그 줄이 빈 줄이면
    if(strcmp(buf, "\r\n") == 0)
    {
      break;
    }
  }
}

// URI가 static인지 dynamic인지 판단
// 실제 파일 경로를 filename에 만들어 넣기
// CGI 인자가 있으면 cgiargs에 분리해서 넣기
int parse_uri(char *uri, char *filename, char *cgiargs)
{
  // uri 보고 정적인지 동적인지 판단
  // cgi-bin 있으면 동적이다/ 없으면 정적
}

void serve_static(int fd, char *filename, int filesize)
{

}

void get_filetype(char *filename, char *filetype)
{

}

void serve_dynamic(int fd, char *filename, char *cgiargs)
{

}

void clienterror(int fd, char *cause, char *errnum, char *shortmsg, char *longmsg)
{

}


int main(int argc, char **argv)
{
  // listen 소켓, 클라이언트 소켓
  int listenfd, connfd;

  // host, port
  char hostname[MAXLINE], port[MAXLINE];

  // 길이
  socklen_t clientlen;

  // 주소 구조체
  struct sockaddr_storage clientaddr;

  /* Check command line args */
  if (argc != 2)
  {
    fprintf(stderr, "usage: %s <port>\n", argv[0]);
    exit(1);
  }

  // 포트로 listen 소켓 만든다
  listenfd = Open_listenfd(argv[1]);

  // 
  while (1)
  {
    clientlen = sizeof(clientaddr);
    
    // 클라이언트 소켓
    connfd = Accept(listenfd, (SA *)&clientaddr, &clientlen); // line:netp:tiny:accept

    // 주소를 문자열로
    Getnameinfo((SA *)&clientaddr, clientlen, hostname, MAXLINE, port, MAXLINE, 0);
    
    // 클라의 port가 연결이 되었다
    printf("Accepted connection from (%s, %s)\n", hostname, port);
    
    doit(connfd);  // line:netp:tiny:doit
    
    Close(connfd); // line:netp:tiny:close
  }
}
