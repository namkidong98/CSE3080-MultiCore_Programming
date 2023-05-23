#include "echo.h"
#include<errno.h>
#include<arpa/inet.h> //htons나 inet_addr과 같은 함수를 사용하기 위한 라이브러리

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

void* thread_operate(void* vargp) {
	int fd = *((int*)vargp); //connfdp에 저장된 connected file descriptor를 받아오고
	Pthread_detach(pthread_self()); //thread가 끝나면 자동적으로 kernel에 의해 reaping되도록 하라는 명령
	Free(vargp); //connfdp는 다 사용했으니 해제하여 memory leaking을 방지
	
	echo(fd);
	
	Close(fd);
	printf("Connection on fd %d closed\n", fd);
	return NULL;

}

int main(int argc, char** argv) {
	int listenfd; //통신을 요청 받을 때 사용되는 소켓(처음 connection request에만 사용)
	int* connfdp; //
	struct sockaddr_in clientaddr; //client의 IP address와 port의 정보를 받아오기 위한 변수
	socklen_t clientlen = sizeof(clientaddr);

	pthread_t tid;

	//입력 예외 처리
	if (argc != 2) { //IP address는 기본적으로 로컬 호스트를 가리키도록 설정되어 있다
		fprintf(stderr, "usage: %s <port>\n", argv[0]);
		exit(0);
	}

	listenfd = Open_listenfd(argv[1]);
	//Getaddrinfo(NULL, port, &hints, &listp); - 첫번째 인자에 hostname or IP address를 가리키는 문자열이 온다
											// - NULL이 오면 로컬 호스트(현재 시스템, 127.0.0.1)을 가리킨다

	while (1) {
		connfdp = (int*)malloc(sizeof(int));
		*connfdp = Accept(listenfd, (SA*)&clientaddr, &clientlen);
		Pthread_create(&tid, NULL, thread_operate, (void*)connfdp);
		printf("Connection from (%s, %d)\n", inet_ntoa(clientaddr.sin_addr), ntohs(clientaddr.sin_port));
	}

	exit(0);
}