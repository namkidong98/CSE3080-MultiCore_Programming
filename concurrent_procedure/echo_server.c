#include "echo.h"
#include<errno.h>
#include<arpa/inet.h> //htons나 inet_addr과 같은 함수를 사용하기 위한 라이브러리

void sigchld_handler(int sig) {
	while (waitpid(-1, 0, WNOHANG) > 0) { //WNOHANG: 어떤 자식이 종료되지 않았더라도 함수는 바로 리턴된다
		printf("child reaped\n"); //child가 reaping되는지 알려주기 위해
	}
	return;
}

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

int main(int argc, char** argv) {
	int listenfd; //통신을 요청 받을 때 사용되는 소켓(처음 connection request에만 사용)
	int connfd; //client랑 연결된 후 직접 정보를 주고 받을 때 사용되는 소켓(connected descriptor)
	struct sockaddr_in clientaddr; //client의 IP address와 port의 정보를 받아오기 위한 변수
	socklen_t clientlen = sizeof(clientaddr);

	//입력 예외 처리
	if (argc != 2) { //IP address는 기본적으로 로컬 호스트를 가리키도록 설정되어 있다
		fprintf(stderr, "usage: %s <port>\n", argv[0]);
		exit(0);
	}
	
	//fork로 프로세스를 생성해서 connection을 연결하고 종료하기 때문에 reap을 안해주면 memory leaking이 발생
	Signal(SIGCHLD, sigchld_handler); //SIGCHLD를 처리해줘야 한다

	listenfd = Open_listenfd(argv[1]);
	//Getaddrinfo(NULL, port, &hints, &listp); - 첫번째 인자에 hostname or IP address를 가리키는 문자열이 온다
											// - NULL이 오면 로컬 호스트(현재 시스템, 127.0.0.1)을 가리킨다

	while (1) {
		connfd = Accept(listenfd, (SA*)&clientaddr, &clientlen);
		printf("Connection from (%s, %d)\n", inet_ntoa(clientaddr.sin_addr), ntohs(clientaddr.sin_port));
		if (Fork() == 0) { //connfd를 처리할 새로운 프로세스(자식 프로세스)를 만들어서 동작 수행
			Close(listenfd); //연결된 프로세스에서는 더 이상 listenfd를 쓰지 않으니 닫아주고

			echo(connfd); //원하는 동작 수행 이후
			Close(connfd); //연결을 끊는다

			printf("Connection (%s, %d) closed\n", inet_ntoa(clientaddr.sin_addr), ntohs(clientaddr.sin_port));
			exit(0);
		}
		Close(connfd); //부모 프로세스 측에서는 client와의 연결을 끊는다
	}

	exit(0);
}