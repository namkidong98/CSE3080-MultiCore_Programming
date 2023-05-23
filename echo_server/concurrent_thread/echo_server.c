#include "echo.h"
#include<errno.h>
#include<arpa/inet.h> //htons�� inet_addr�� ���� �Լ��� ����ϱ� ���� ���̺귯��

void echo(int connfd) { //client�κ��� �Է¹��� ���� �״�� client���� �ٽ� �����ִ� ������ �ϴ� �Լ�
	int n;
	char buf[MAXLINE];
	rio_t rio;

	Rio_readinitb(&rio, connfd); //rio�� �ʱ�ȭ
	while ((n = Rio_readlineb(&rio, buf, MAXLINE)) != 0) { //Robustly read a text line (buffered) : READ from client
		printf("server received %d bytes\n", n);
		Rio_writen(connfd, buf, n); //WRITE to client
	}
}

void* thread_operate(void* vargp) {
	int fd = *((int*)vargp); //connfdp�� ����� connected file descriptor�� �޾ƿ���
	Pthread_detach(pthread_self()); //thread�� ������ �ڵ������� kernel�� ���� reaping�ǵ��� �϶�� ���
	Free(vargp); //connfdp�� �� ��������� �����Ͽ� memory leaking�� ����
	
	echo(fd);
	
	Close(fd);
	printf("Connection on fd %d closed\n", fd);
	return NULL;

}

int main(int argc, char** argv) {
	int listenfd; //����� ��û ���� �� ���Ǵ� ����(ó�� connection request���� ���)
	int* connfdp; //
	struct sockaddr_in clientaddr; //client�� IP address�� port�� ������ �޾ƿ��� ���� ����
	socklen_t clientlen = sizeof(clientaddr);

	pthread_t tid;

	//�Է� ���� ó��
	if (argc != 2) { //IP address�� �⺻������ ���� ȣ��Ʈ�� ����Ű���� �����Ǿ� �ִ�
		fprintf(stderr, "usage: %s <port>\n", argv[0]);
		exit(0);
	}

	listenfd = Open_listenfd(argv[1]);
	//Getaddrinfo(NULL, port, &hints, &listp); - ù��° ���ڿ� hostname or IP address�� ����Ű�� ���ڿ��� �´�
											// - NULL�� ���� ���� ȣ��Ʈ(���� �ý���, 127.0.0.1)�� ����Ų��

	while (1) {
		connfdp = (int*)malloc(sizeof(int));
		*connfdp = Accept(listenfd, (SA*)&clientaddr, &clientlen);
		Pthread_create(&tid, NULL, thread_operate, (void*)connfdp);
		printf("Connection from (%s, %d)\n", inet_ntoa(clientaddr.sin_addr), ntohs(clientaddr.sin_port));
	}

	exit(0);
}