/* $begin tinymain */
/*
 * tiny.c - A simple, iterative HTTP/1.0 Web server that uses the
 *     GET method to serve static and dynamic content.
 *
 * Updated 11/2019 droh
 *   - Fixed sprintf() aliasing issue in serve_static(), and clienterror().
 */
#include "csapp.h"
#include <string.h>


// cd C:\Users\JCB\week8\webproxy_lab_docker
// docker build -f .devcontainer\Dockerfile -t proxylab-dev .devcontainer
// docker run --rm -it -p 8001:8001 -v C:\Users\JCB\week8\webproxy_lab_docker:/workspaces/webproxy_lab_docker -w /workspaces/webproxy_lab_docker/webproxy-lab/tiny proxylab-dev bash
// ls
// make
// ./tiny 8000


void doit(int fd);
void read_requesthdrs(rio_t *rp);
int parse_uri(char *uri, char *filename, char *cgiargs);
void serve_static(int fd, char *filename, int filesize, int is_head);
void get_filetype(char *filename, char *filetype);
void serve_dynamic(int fd, char *filename, char *cgiargs, int is_head);
void clienterror(int fd, char *cause, char *errnum, char *shortmsg,
                 char *longmsg);

void doit(int fd)
{
    int is_static;
    int is_head;
    struct stat sbuf;
    ssize_t n;
    rio_t rio;
    char buf[MAXLINE], method[MAXLINE], uri[MAXLINE], version[MAXLINE];
    char filename[MAXLINE], cgiargs[MAXLINE];

    Rio_readinitb(&rio, fd);
    n = Rio_readlineb(&rio, buf, MAXLINE);
    if (n <= 0) {
        return;
    }

    printf("%s", buf);

    if (sscanf(buf, "%s %s %s", method, uri, version) != 3) {
        clienterror(fd, buf, "400", "Bad Request",
                    "Tiny could not parse request line");
        return;
    }

    if (strcasecmp(method, "GET") && strcasecmp(method, "HEAD")) {
        clienterror(fd, method, "501", "Not Implemented",
                    "Tiny does not implement this method");
        return;
    }

    //method가 Head면 0반환
    //method가 GET이면 1반환
    is_head = !strcasecmp(method, "HEAD");
    read_requesthdrs(&rio);
    is_static = parse_uri(uri, filename, cgiargs);
    if (stat(filename, &sbuf) < 0) {
        clienterror(fd, filename, "404", "Not found",
                    "Tiny couldn't find this file");
        return;
    }

    if (is_static) {
        if (!(S_ISREG(sbuf.st_mode)) || !(S_IRUSR & sbuf.st_mode)) {
            clienterror(fd, filename, "403", "Forbidden",
                        "Tiny couldn't read the file");
            return;
        }
        serve_static(fd, filename, sbuf.st_size, is_head);
    } else {
        if (!(S_ISREG(sbuf.st_mode)) || !(S_IXUSR & sbuf.st_mode)) {
            clienterror(fd, filename, "403", "Forbidden",
                        "Tiny couldn't run the CGI program");
            return;
        }
        serve_dynamic(fd, filename, cgiargs, is_head);
    }
}

void read_requesthdrs(rio_t *rp)
{
    char buf[MAXLINE];

    do {
        if (Rio_readlineb(rp, buf, MAXLINE) <= 0) {
            break;
        }
        printf("%s", buf); // 요청 buf 바로 출력
    } while (strcmp(buf, "\r\n")); //이게 0이면 반복종료
}

int parse_uri(char *uri, char *filename, char *cgiargs)
{
    char *ptr;

    if (!strstr(uri, "cgi-bin")) {
        strcpy(cgiargs, "");
        strcpy(filename, ".");
        strcat(filename, uri);
        if (uri[strlen(uri) - 1] == '/') {
            strcat(filename, "home.html");
        }
        return 1;
    }

    ptr = strchr(uri, '?');
    if (ptr) {
        strcpy(cgiargs, ptr + 1);
        *ptr = '\0';
    } else {
        strcpy(cgiargs, "");
    }

    strcpy(filename, ".");
    strcat(filename, uri);
    return 0;
}

void serve_static(int fd, char *filename, int filesize, int is_head)
{
    int srcfd;
    int n;
    int remaining;
    char *srcbuf;
    char *p;
    char filetype[MAXBUF], buf[MAXBUF];

    get_filetype(filename, filetype);
    p = buf;
    remaining = sizeof(buf);

    n = snprintf(p, remaining, "HTTP/1.0 200 OK\r\n");
    p += n;
    remaining -= n;

    n = snprintf(p, remaining, "Server: Tiny Web Server\r\n");
    p += n;
    remaining -= n;

    n = snprintf(p, remaining, "Connection: close\r\n");
    p += n;
    remaining -= n;

    n = snprintf(p, remaining, "Content-length: %d\r\n", filesize);
    p += n;
    remaining -= n;

    snprintf(p, remaining, "Content-type: %s\r\n\r\n", filetype);

    Rio_writen(fd, buf, strlen(buf));

    if (is_head) {
        return;
    }

    /*
    // Mmap이 파일 내용을 그냥 가상 메모리에 매핑해준다(그래서 read 필요없음)
     * Original version using mmap:
     * srcfd = Open(filename, O_RDONLY, 0); // 파일 열어서
     * srcp = Mmap(0, filesize, PROT_READ, MAP_PRIVATE, srcfd, 0); // 파일을 메모리 주소에 연결
     * Close(srcfd); // 파일 닫는다
     * Rio_writen(fd, srcp, filesize); // 파일 내용을 그대로 클라이언트 소켓으로 보낸다
     * Munmap(srcp, filesize);
     */

    //이전 버전 
    //Mmap()으로 파일을 가상 메모리에 매핑
    //그 매핑된 메모리 주소를 바로 Rio_writen으로 소켓에 전송
    //끝나면 Munmap()으로 해제

    //현재 버전
    //Malloc으로 직접 버퍼를 만든다
    //Rio_readn()으로 파일 내용을 그 버퍼에 읽어온다
    //Rio_writen()으로 클라이언트에 보냄
    //Free()로 버퍼 해제
    srcfd = Open(filename, O_RDONLY, 0);
    srcbuf = Malloc(filesize);
    Rio_readn(srcfd, srcbuf, filesize);
    Close(srcfd);

    Rio_writen(fd, srcbuf, filesize);
    Free(srcbuf);
}

void get_filetype(char *filename, char *filetype)
{
    if (strstr(filename, ".html")) {
        strcpy(filetype, "text/html");
    } else if (strstr(filename, ".gif")) {
        strcpy(filetype, "image/gif");
    } else if (strstr(filename, ".png")) {
        strcpy(filetype, "image/png");
    } else if (strstr(filename, ".jpg")) {
        strcpy(filetype, "image/jpg");
    }
    else if (strstr(filename, ".mpg")) {
        strcpy(filetype, "video/mpg");
    }
    else if (strstr(filename, ".mp4")) {
        strcpy(filetype, "video/mp4");
    } 
    else {
        strcpy(filetype, "text/plain");
    }
}

void serve_dynamic(int fd, char *filename, char *cgiargs, int is_head)
{
    char buf[MAXLINE], *emptylist[] = { NULL };

    sprintf(buf, "HTTP/1.0 200 OK\r\n");
    Rio_writen(fd, buf, strlen(buf));
    sprintf(buf, "Server: Tiny Web Server\r\n");
    Rio_writen(fd, buf, strlen(buf));
    sprintf(buf, "\r\n");
    Rio_writen(fd, buf, strlen(buf));

    if (is_head) {
        return;
    }

    if (Fork() == 0) {
        setenv("QUERY_STRING", cgiargs, 1);
        Dup2(fd, STDOUT_FILENO);
        Execve(filename, emptylist, environ);
    }
    Wait(NULL);
}

void clienterror(int fd, char *cause, char *errnum, char *shortmsg,
                 char *longmsg)
{
    char buf[MAXLINE], body[MAXBUF];

    body[0] = '\0';
    sprintf(body, "<html><title>Tiny Error</title>");
    sprintf(body + strlen(body), "<body bgcolor=\"ffffff\">\r\n");
    sprintf(body + strlen(body), "%s: %s\r\n", errnum, shortmsg);
    sprintf(body + strlen(body), "<p>%s: %s\r\n", longmsg, cause);
    sprintf(body + strlen(body), "<hr><em>The Tiny web server</em>\r\n");

    sprintf(buf, "HTTP/1.0 %s %s\r\n", errnum, shortmsg);
    Rio_writen(fd, buf, strlen(buf));
    sprintf(buf, "Content-type: text/html\r\n");
    Rio_writen(fd, buf, strlen(buf));
    sprintf(buf, "Content-length: %d\r\n\r\n", (int)strlen(body));
    Rio_writen(fd, buf, strlen(buf));
    Rio_writen(fd, body, strlen(body));
}

int main(int argc, char **argv)
{
    int listenfd, connfd;
    socklen_t clientlen;
    struct sockaddr_storage clientaddr;
    char hostname[MAXLINE], port[MAXLINE];

    if (argc != 2) {
        fprintf(stderr, "usage: %s <port>\n", argv[0]);
        exit(1);
    }

    listenfd = Open_listenfd(argv[1]);
    while (1) {
        clientlen = sizeof(clientaddr);
        connfd = Accept(listenfd, (SA *)&clientaddr, &clientlen);
        Getnameinfo((SA *)&clientaddr, clientlen, hostname, MAXLINE, port,
                    MAXLINE, 0);
        printf("Accepted connection from (%s, %s)\n", hostname, port);
        doit(connfd);
        Close(connfd);
    }
}
/* $end tinymain */
