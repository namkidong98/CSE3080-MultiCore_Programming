#include "echo.h"
#include<errno.h>
#include<arpa/inet.h> //htons나 inet_addr과 같은 함수를 사용하기 위한 라이브러리

int main(int argc, char** argv) {
	int clientfd; //client-side에서 통신에 사용되는 소켓
	struct sockaddr_in serveraddr; //client-side에서는 server의 IP address, port를 저장할 변수를 만들어 놓는다

	char buf[MAXLINE]; //server에 보낼 문자열을 표준 입력에서 받아 저장하고 있는 변수
	
	//1. Socket 함수를 이용하여 clientfd를 소켓 파일 디스크립터로 만든다
	clientfd = Socket(AF_INET, SOCK_STREAM, 0); //domain, type, protocol(always 0)

	//2. 입력받은 인자인 IP address와 port를 적절히 변환하여 serveraddr에 저장한다
	serveraddr.sin_family = AF_INET;
	serveraddr.sin_addr.s_addr = inet_addr(argv[1]); //inet_addr는 127.0.0.1과 같은 IP address를 정수로 변경해주는 함수
	serveraddr.sin_port = htons(atoi(argv[2])); //실행 명령어 뒤에 2번째 인자에 온 port number를 Big Endian으로 읽어라

	//3. Connect 함수를 이용하여 서버에 연결 요청을 보낸다(=connection request)
	Connect(clientfd, (SA*) &serveraddr, sizeof(serveraddr));


	//------------------이제부터는 서버와 연결된 상태이다--------------//


	while (Fgets(buf, MAXLINE, stdin) != NULL) { //1. Terminal에 온 표준 입력을 buf에 저장

		//2. Write 함수를 이용하여 Terminal input을 server에 보낸다
		Write(clientfd, buf, strlen(buf));

		//3. Read 함수를 이용하여 server에서 보내온 정보를 buf에 읽어서 저장한다
		Read(clientfd, buf, MAXLINE);

		//4. Terminal에 읽어온 buf를 출력한다(= echo)
		Fputs(buf, stdout);
	}

	//5. 입력을 더이상 안해서 끊으려고 할 때 Close로 연결되었던 clientfd를 닫아서 server쪽에서 EOF를 읽어들이게 만든다
	Close(clientfd);

	exit(0);
}