/* $begin tinymain */
/*
 * tiny.c - A simple, iterative HTTP/1.0 Web server that uses the
 *     GET method to serve static and dynamic content.
 *
 * Updated 11/2019 droh
 *   - Fixed sprintf() aliasing issue in serve_static(), and clienterror().
 */
#include "csapp.h"
#include "string.h"
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

  //읽은 바이트 수 저장
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

  // 정적이다
  if(strstr(uri, "cgi-bin") == 0)
  {
    // 정적 콘텐츠는 cgi 인자가 없으니까 비워두고
    strcpy(cgiargs, "");

    if(strcmp(uri, "/") == 0) // 맞으면 0반환
    {
      strcpy(filename, ".");

      strcat(filename, uri);
      strcat(filename, "home.html");
      
      return 1;
    }

    // filename은 앞에 .을 붙여 경로를 만들어준다
    strcpy(filename, ".");

    strcat(filename, uri);

    return 1;
  }

  // 동적이다
  else
  {
    // uri안에 ?가 없다
    if(strstr(uri, "?") == NULL)
    {
      //cgiargs(인자)에 빈 값대입
      strcpy(cgiargs, "");

      //filename에 ./cgi-bin/adder 저장
      strcpy(filename, ".");
      strcat(filename, uri);
      return 0;
    }


    // ?의 위치를 받을 포인터
    char* ptr;

    // ptr이 uri내에서 ? 위치를 가리킨다
    ptr = strchr(uri, '?');

    // ptr+1 즉 ?위치 그 다음부터를 cgiargs에 저장한다
    strcpy(cgiargs, ptr + 1);
    *ptr = '\0';

    strcpy(filename, ".");
    strcat(filename, uri);

    return 0;

  }
}

// 이 파일을 HTTP 응답 형식으로 포장해서 소켓 fd로 내보내는 함수
// fd : 클라이언트와 연결된 소켓
// filename : 보낼 실제 파일 경로
// filesize : 응답 헤더의 Content-length에 넣을 파일 크기
void serve_static(int fd, char *filename, int filesize)
{

  int srcfd;

  //헤더
  char *srcp;
  char filetype[MAXSIZE], buf[MAXSIZE];

  buf[0] = '\0';

  //filetype에 Meme 채운다
  //ex) image/gif
  //file type보내야 클라이언트가 뭔지 알수있음
  get_filetype(filename, filetype);
  
  // 헤더를 buf에 저장
  sprintf(buf, "HTTP/1.0 200 OK\r\n");
  sprintf(buf + strlen(buf), "Server: Tiny Web Server\r\n");
  sprintf(buf + strlen(buf), "Connection: close\r\n");
  sprintf(buf + strlen(buf), "Content-Length: %d\r\n", filesize);
  sprintf(buf + strlen(buf), "Content-type: %s\r\n\r\n", filetype);
  
  //클라에게 헤더 보낸다
  Rio_writen(fd, buf, strlen(buf));

  //서버 터미널에 출력
  printf("Response headers:\n");
  printf("%s", buf);

  //진짜 보낼 파일을 연다
  //서버 디스크에 있는 home.html 파일을 연다
  //srcfd = 파일자체
  srcfd = Open(filename, 0);

  //파일 내용을 메모리 주소 공간에 연결한다
  //filesize 만큼 매핑, PROT_READ : 읽기만 할거다, MAP_PRIVATE : private mapping, srcfd : 아까 연파일, 파일 시작 위치부터ㅇ
  srcp = Mmap(0, filesize, PROT_READ, MAP_PRIVATE, srcfd, 0); // 파일 내용의 시작주소

  //파일 디스크립터 닫는다
  //파일 내용 이미 Mmap으로 메모리에 붙여놓음
  //파일 여는 역할 끝남
  Close(srcfd);

  // 바디를 브라우저에 보낸다
  Rio_writen(fd, srcp, filesize);

  //메모리에 붙여놓은 파일 매핑 해제한다
  Munmap(srcp, filesize);
}

void get_filetype(char *filename, char *filetype)
{
  //filename안에 .html이 있으면
  if(strstr(filename, ".html"))
  {
    strcpy(filetype, "text/html");
  }
  else if(strstr(filename, ".gif"))
  {
    strcpy(filetype, "image/gif");
  }
  else if(strstr(filename, ".png"))
  {
    strcpy(filetype, "image/png");
  }
  else if(strstr(filename, ".jpg"))
  {
    strcpy(filetype, "image/jpg");
  }
  else
  {
    strcpy(filetype, "text/plain");
  }

}

void serve_dynamic(int fd, char *filename, char *cgiargs)
{
  //cgi프로그램 실행해서 그 프로그램의 출력결과를 보내는 함수

  //fork로 cgi 프로세스 실행

  char buf[MAXLINE], *emptylist[] = {NULL};

  //buf에 저장
  sprintf(buf, "HTTP.1.0 200 OK\r\n");
  Rio_writen(fd, buf, strlen(buf)); // 버퍼 덮어쓰기
  sprintf(buf, "Server : Tiny Web Server\r\n");
  Rio_writen(fd, buf, strlen(buf)); // 버퍼 덮어쓰기

  // 프로세스를 둘로 쪼갠다(부모 프로세스, 자식 프로세스)
  if(Fork() == 0)
  {
    //QUERY_STRING에 cgiargs 넣는다
    setenv("QUERY_STRING", cgiargs, 1);

    // 자식 프로세스의 표준출력(STDOUT_FILENO)을 클라이언트 소켓fd로 바꿔치기한다
    // 즉 원래는 printf()하면 터미널로 출력되었을텐데, 이제는 브라우저로 간다
    Dup2(fd, STDOUT_FILENO); // STDOUT_FILENO가 원래 가리키던 곳 대신, 이제 fd 소켓을 가리키게 해라!
    Execve(filename, emptylist, environ);
  }
  Wait(NULL);
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
