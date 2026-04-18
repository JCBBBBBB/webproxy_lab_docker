#include "../csapp.h"

void echo(int connfd);

//클라와 실제 통신하고 받은 내용을 그대로 돌려주는 역할
void echo(int connfd) // 현재 클라와 연결된 소켓번호
{   
    // 한번 읽을 때 몇바이트 읽었는지 저장
    size_t n;

    // 클라이언트가 보낸 문자열을 담는 버퍼
    char buf[MAXLINE];

    // RIO 읽기 상태를 관리하는 구조체
    rio_t rio;

    // connfd에서 들어오는 데이터를 RIO 방식으로 읽겠다는 준비
    Rio_readinitb(&rio, connfd);

    // 읽은 바이트 수를 n에 저장
    // 읽은게 0이 아니면 계속 반복
    while ((n = Rio_readlineb(&rio, buf, MAXLINE)) != 0) { //Rio_readlineb -> hello\n

        //서버 실제로 몇바이트 받았는지 출력
        printf("server received %zu bytes %c\n", n, buf);

        // 클라가 보낸 buf의 내용을 다시 클라에게 보낸다
        Rio_writen(connfd, buf, n); // n바이트를 끝까지 확실하게 써주는 함수
    }
}

int main(int argc, char **argv)
{
    int listenfd, connfd;
    socklen_t clientlen; // clientaddr의 길이
    struct sockaddr_storage clientaddr; // 클라이언트의 주소를 담는 구조체
    char hostname[MAXLINE], port[MAXLINE]; // echo, 8080

    if (argc != 2) { // 2개가 아니라면 오류
        fprintf(stderr, "usage: %s <port>\n", argv[0]);
        exit(1);
    }

    // 서버용 듣기 소켓을 만든다
    // 새 연결을 기다리는 소켓
    listenfd = Open_listenfd(argv[1]); // 8080포트로 listen 소켓을 생성한다

    while (1) 
    {
        clientlen = sizeof(clientaddr);

        //클라이언트와 대화할 전용 소켓을 만든다
        // Accept가 접속한 클라이언트 주소 정보를 clientaddr에 채워주고
        // (struct sockaddr *) 라는 뜻
        connfd = Accept(listenfd, (SA *)&clientaddr, &clientlen); // 클라가 connect한걸 연결해주는 거

        // 접속한 클라이언트 정보를 문자열로 바꾼다
        Getnameinfo((SA *)&clientaddr, clientlen, hostname, MAXLINE, port, MAXLINE, 0);

        //서버가 클라이언트 1개 연결을 받았다는 뜻
        // ex ) Connected to (localhost, 51240)
        // 서버는 8080 포트에서 기다리고 있는데 클라는 127.0.0.1:51240이라는 주소로 접속함
        printf("Connected to (%s, %s)\n", hostname, port); 

        // 클라이언트를 실제로 응대
        echo(connfd);
        Close(connfd);
    }
}
