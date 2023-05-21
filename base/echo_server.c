#include "echo.h"
#include<errno.h>
#include<arpa/inet.h> //htons�� inet_addr�� ���� �Լ��� ����ϱ� ���� ���̺귯��

typedef struct sockaddr SA;

int main(int argc, char** argv) {
	struct sockaddr_in listenaddr; //server�� IP address�� port�� ������ ���� �ִ� ����, 
	struct sockaddr_in clientaddr; //client�� IP address�� port�� ������ �޾ƿ��� ���� ����
	socklen_t clientlen = sizeof(clientaddr); 
	
	size_t n; //client���� �޾ƿ��� ���ڿ��� ũ�⸦ ��Ÿ���� ����
	char buf[MAXLINE]; //client���� �޾ƿ� ���ڿ��� �����ϴ� ����
	int opt = 1;

	int listenfd; //����� ��û ���� �� ���Ǵ� ����(ó�� connection request���� ���)
	int connfd; //client�� ����� �� ���� ������ �ְ� ���� �� ���Ǵ� ����(connected descriptor)

	//�Է� ���� ó��
	if (argc != 3) {
		fprintf(stderr, "usage: %s <IP> <port>\n", argv[0]);
		exit(0);
	}

	//1. Socket �Լ��� �̿��Ͽ� listenfd�� ���� ���� ��ũ���ͷ� �����
	listenfd = Socket(AF_INET, SOCK_STREAM, 0); //domain, type, protocol(always 0)

		//Ŀ���� ���� �ּҸ� �ٷ� �������� �������� bind�� ����ó���� �ߴ� ��츦 �����ֱ� ���� ��ġ
		Setsockopt(listenfd, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt)); //SO_REUSEADDR�� opt�� ����� 1�� ����

	//2. �Է¹��� ������ IP address�� port�� ������ ��ȯ�Ͽ� listenaddr�� �����Ѵ�
	listenaddr.sin_family = AF_INET;
	listenaddr.sin_addr.s_addr = inet_addr(argv[1]); //inet_addr�� 127.0.0.1�� ���� IP address�� ������ �������ִ� �Լ�
	listenaddr.sin_port = htons(atoi(argv[2])); //���� ��ɾ� �ڿ� 2��° ���ڿ� �� port number�� Big Endian���� �о��

	//3. listenfd��� ���� ��ũ���Ϳ� server�� ������ ���� �ִ� listenaddr�� �����Ų��
	Bind(listenfd, (SA*)&listenaddr, sizeof(listenaddr));

	//4. Listen �Լ��� �̿��Ͽ� listenfd�� server-side ���� ��ũ���Ͷ�� ���� ������ش�
	  //(socketfd�� ����� by default, client-side�� �����ϱ� ����)
	Listen(listenfd, 5); //sockfd, backlog������ ���ڸ� �޴µ� backlog�� pending queue�� maximum length�� �ǹ��Ѵ�

	//-------------------���� server-side�� connection�� �� �غ� �� �����̴�---------------//

	while (1) {
		//5. Accept �Լ��� �̿��Ͽ� connection request�� ��ٸ��ٰ� ��û�� ���� clientaddr, clientlen�� ������ �ް� ��,
		   //connfd�� connected descriptor�� ��ȯ
		connfd = Accept(listenfd, (SA*)&clientaddr, &clientlen); 

		//---------------------------���ĺ��ʹ� echo ����� �����ϱ� ���� �ڵ��------------------//
		printf("Connection from (%s, %d)\n", inet_ntoa(clientaddr.sin_addr), ntohs(clientaddr.sin_port));

		//1. Read �Լ��� ����Ͽ� buf�� MAXLINE��ŭ �Է��� �о�´� --> n���� ���� �������� ũ�Ⱑ ����ȴ�
		while ((n = Read(connfd, buf, MAXLINE)) != 0) { //n == 0 �̸� EOF�� �޾ƿ� ��(�Է��� ���� ����)
			printf("server received %d bytes\n", (int)n);

			//2. Write �Լ��� ����Ͽ� buf�� �ִ� ������ n��ŭ server�� ������ 
			Write(connfd, buf, n); //
		}

		//3. ������ �����ؾ� �� ����(EOF)�� �Ǹ�, Close �Լ��� ���� connfd�� ����� client���� ������ �����Ѵ�
		Close(connfd);
		printf("Connection closed\n");
	}

	exit(0);
}