#include "echo.h"
#include<errno.h>
#include<arpa/inet.h> //htons나 inet_addr과 같은 함수를 사용하기 위한 라이브러리

typedef struct sockaddr SA;

int main(int argc, char** argv) {
	struct sockaddr_in listenaddr; //server의 IP address와 port의 정보를 갖고 있는 변수, 
	struct sockaddr_in clientaddr; //client의 IP address와 port의 정보를 받아오기 위한 변수
	socklen_t clientlen = sizeof(clientaddr); 
	
	size_t n; //client에서 받아오는 문자열의 크기를 나타내는 변수
	char buf[MAXLINE]; //client에서 받아올 문자열을 저장하는 변수
	int opt = 1;

	int listenfd; //통신을 요청 받을 때 사용되는 소켓(처음 connection request에만 사용)
	int connfd; //client랑 연결된 후 직접 정보를 주고 받을 때 사용되는 소켓(connected descriptor)

	//입력 예외 처리
	if (argc != 3) {
		fprintf(stderr, "usage: %s <IP> <port>\n", argv[0]);
		exit(0);
	}

	//1. Socket 함수를 이용하여 listenfd를 소켓 파일 디스크립터로 만든다
	listenfd = Socket(AF_INET, SOCK_STREAM, 0); //domain, type, protocol(always 0)

		//커널이 소켓 주소를 바로 해제하지 않음으로 bind의 예외처리가 뜨는 경우를 막아주기 위한 장치
		Setsockopt(listenfd, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt)); //SO_REUSEADDR를 opt에 저장된 1로 세팅

	//2. 입력받은 인자인 IP address와 port를 적절히 변환하여 listenaddr에 저장한다
	listenaddr.sin_family = AF_INET;
	listenaddr.sin_addr.s_addr = inet_addr(argv[1]); //inet_addr는 127.0.0.1과 같은 IP address를 정수로 변경해주는 함수
	listenaddr.sin_port = htons(atoi(argv[2])); //실행 명령어 뒤에 2번째 인자에 온 port number를 Big Endian으로 읽어라

	//3. listenfd라는 소켓 디스크립터와 server의 정보를 갖고 있는 listenaddr를 연결시킨다
	Bind(listenfd, (SA*)&listenaddr, sizeof(listenaddr));

	//4. Listen 함수를 이용하여 listenfd가 server-side 파일 디스크립터라는 것을 명시해준다
	  //(socketfd를 만들면 by default, client-side라 가정하기 때문)
	Listen(listenfd, 5); //sockfd, backlog순으로 인자를 받는데 backlog는 pending queue의 maximum length를 의미한다

	//-------------------이제 server-side는 connection이 될 준비가 된 상태이다---------------//

	while (1) {
		//5. Accept 함수를 이용하여 connection request를 기다리다가 요청이 오면 clientaddr, clientlen에 정보를 받고난 후,
		   //connfd를 connected descriptor로 반환
		connfd = Accept(listenfd, (SA*)&clientaddr, &clientlen); 

		//---------------------------이후부터는 echo 기능을 구현하기 위한 코드들------------------//
		printf("Connection from (%s, %d)\n", inet_ntoa(clientaddr.sin_addr), ntohs(clientaddr.sin_port));

		//1. Read 함수를 사용하여 buf에 MAXLINE만큼 입력을 읽어온다 --> n에는 읽은 데이터의 크기가 저장된다
		while ((n = Read(connfd, buf, MAXLINE)) != 0) { //n == 0 이면 EOF를 받아온 것(입력이 없는 상태)
			printf("server received %d bytes\n", (int)n);

			//2. Write 함수를 사용하여 buf에 있는 정보를 n만큼 server에 보낸다 
			Write(connfd, buf, n); //
		}

		//3. 연결을 종료해야 할 조건(EOF)이 되면, Close 함수를 통해 connfd에 연결된 client와의 연결을 종료한다
		Close(connfd);
		printf("Connection closed\n");
	}

	exit(0);
}