#include "../csapp.h"

//서버 실행:
// ./echo 8080

// 클라이언트 실행:
// ./client 127.0.0.1 8080



int main(int argc, char **argv)
{
    int clientfd; // 서버에 연결된 클라의 소켓 번호

    // host : 접속할 서버 주소
    // prot : 접속할 포트 번호
    // buf[MAXLINE] : 입력한 문자열이나 서버가 보내준 문자열을 잠시 담는 공간
    char *host, *port, buf[MAXLINE]; // ./client 127.0.0.1 8080

    // 소켓에서 한 줄씩 안전하게 읽기 위한 준비물
    rio_t rio;
    
    //실행 형식이 맞는지 검사
    if (argc != 3) {
        fprintf(stderr, "usage: %s <host> <port>\n", argv[0]);
        exit(1);
    }

    // host에 있는 주소
    host = argv[1]; // 127.0.0.1

    // 포트 번호
    port = argv[2]; // 8080

    // 서버에 연결을 시도해서, 성공하면 연결된 소켓 디스크립터를 반환한다.
    // 여기서 반환된 clientfd라는 소켓 번호를  가지고 이후에 서버와 읽고 쓰기를 한다
    clientfd = Open_clientfd(host, port);

    //서버 쪽에서 오는 데이터를 안전하게 읽기 위한 준비
    Rio_readinitb(&rio, clientfd);

    // 사용자가 입력하는 동안 계속
    while (Fgets(buf, MAXLINE, stdin) != NULL) {
        Rio_writen(clientfd, buf, strlen(buf)); // 사용자가 입력한 문자열을 서버에 보낸다   ex) hello
        Rio_readlineb(&rio, buf, MAXLINE); // 서버가 보내는 한 줄을 읽어서 buf에 저장한다   ex) hello buf에 저장
        Fputs(buf, stdout);// buf에 들어있는 문자열을 화면에 출력한다 ex) hello 출력
    }

    // 사용자 입력 끝내면
    // 서버와 연결을 닫는다
    Close(clientfd);
    return 0;
}
