#include "echo.h"
#include<errno.h>
#include<arpa/inet.h> //htons나 inet_addr과 같은 함수를 사용하기 위한 라이브러리
#include <sys/select.h> //select함수나 fdset에 관한 함수들

void echo(int connfd) { //client로부터 입력받은 것을 그대로 client에게 다시 보내주는 역할을 하는 함수
	int n;
	char buf[MAXLINE];
	rio_t rio;

	Rio_readinitb(&rio, connfd); //rio를 초기화
	while ((n = Rio_readlineb(&rio, buf, MAXLINE)) != 0) { //Robustly read a text line (buffered) : READ from client
		printf("server received %d bytes\n", n);
		Rio_writen(connfd, buf, n); //WRITE to client
	}
}

void command() {
	char buf[MAXLINE];
	if (!Fgets(buf, MAXLINE, stdin)) exit(0); //terminal에서 입력받은 것이 없으면 종료(에러)
	printf("%s", buf);	//terminal에서 입력받은 것을 그대로 출력
}

int main(int argc, char** argv) {
	int listenfd; //통신을 요청 받을 때 사용되는 소켓(처음 connection request에만 사용)
	int connfd; //client랑 연결된 후 직접 정보를 주고 받을 때 사용되는 소켓(connected descriptor)
	struct sockaddr_in clientaddr; //client의 IP address와 port의 정보를 받아오기 위한 변수
	socklen_t clientlen = sizeof(clientaddr);

	fd_set read_set, ready_set;

	//입력 예외 처리
	if (argc != 2) { //IP address는 기본적으로 로컬 호스트를 가리키도록 설정되어 있다
		fprintf(stderr, "usage: %s <port>\n", argv[0]);
		exit(0);
	}

	listenfd = Open_listenfd(argv[1]);
	//Getaddrinfo(NULL, port, &hints, &listp); - 첫번째 인자에 hostname or IP address를 가리키는 문자열이 온다
											// - NULL이 오면 로컬 호스트(현재 시스템, 127.0.0.1)을 가리킨다

	FD_ZERO(&read_set);					//read_set을 초기화
	FD_SET(STDIN_FILENO, &read_set);	//stdin(terminal input)을 read_set에 추가
	FD_SET(listenfd, &read_set);		//listenfd를 read_set에 추가

	while (1) {
		ready_set = read_set;			//select함수는 인자로 받은 fdset을 변경시키기 때문에 read_set말고 다른 fdset이 필요
		
		Select(listenfd + 1, &ready_set, NULL, NULL, NULL);		//입출력 가능한 디스크립터의 개수를 반환, 에러의 경우 -1을 반환
		//첫 번째 인자: 모니터링할 디스크립터의 개수			두 번째 인자: 읽기 이벤트가 발생한 디스크립터 집합
		//세 번째 인자: 쓰기 이벤트가 발생한 디스크립터 집합	네 번째 인자: 예외 이벤트가 발생한 디스크립터 집합
		//다섯 번째 인자: timeout에 지정된 시간동안 대기(blocking을 통제, NULL이면 무한정 blocking)
		
		if (FD_ISSET(STDIN_FILENO, &ready_set)) //select로 읽기 이벤트가 발생한 디스크립터 집합(ready_set)을 갱신한 후, 거기에 STDIN이 있다면
			command();
		if (FD_ISSET(listenfd, &ready_set)) {	//select로 읽기 이벤트가 발생한 디스크립터 집합(ready_set)을 갱신한 후, 거기에 listenfd가 있다면
			connfd = Accept(listenfd, (SA*)&clientaddr, &clientlen);
			printf("Connection from (%s, %d)\n", inet_ntoa(clientaddr.sin_addr), ntohs(clientaddr.sin_port));

			echo(connfd); //원하는 동작 수행 이후
			Close(connfd); //연결을 끊는다

			printf("Connection (%s, %d) closed\n", inet_ntoa(clientaddr.sin_addr), ntohs(clientaddr.sin_port));
		}
		
	}

	exit(0);
}