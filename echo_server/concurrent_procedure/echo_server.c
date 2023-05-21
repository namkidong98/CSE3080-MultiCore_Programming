#include "echo.h"
#include<errno.h>
#include<arpa/inet.h> //htons�� inet_addr�� ���� �Լ��� ����ϱ� ���� ���̺귯��

void sigchld_handler(int sig) {
	while (waitpid(-1, 0, WNOHANG) > 0) { //WNOHANG: � �ڽ��� ������� �ʾҴ��� �Լ��� �ٷ� ���ϵȴ�
		printf("child reaped\n"); //child�� reaping�Ǵ��� �˷��ֱ� ����
	}
	return;
}

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

int main(int argc, char** argv) {
	int listenfd; //����� ��û ���� �� ���Ǵ� ����(ó�� connection request���� ���)
	int connfd; //client�� ����� �� ���� ������ �ְ� ���� �� ���Ǵ� ����(connected descriptor)
	struct sockaddr_in clientaddr; //client�� IP address�� port�� ������ �޾ƿ��� ���� ����
	socklen_t clientlen = sizeof(clientaddr);

	//�Է� ���� ó��
	if (argc != 2) { //IP address�� �⺻������ ���� ȣ��Ʈ�� ����Ű���� �����Ǿ� �ִ�
		fprintf(stderr, "usage: %s <port>\n", argv[0]);
		exit(0);
	}
	
	//fork�� ���μ����� �����ؼ� connection�� �����ϰ� �����ϱ� ������ reap�� �����ָ� memory leaking�� �߻�
	Signal(SIGCHLD, sigchld_handler); //SIGCHLD�� ó������� �Ѵ�

	listenfd = Open_listenfd(argv[1]);
	//Getaddrinfo(NULL, port, &hints, &listp); - ù��° ���ڿ� hostname or IP address�� ����Ű�� ���ڿ��� �´�
											// - NULL�� ���� ���� ȣ��Ʈ(���� �ý���, 127.0.0.1)�� ����Ų��

	while (1) {
		connfd = Accept(listenfd, (SA*)&clientaddr, &clientlen);
		printf("Connection from (%s, %d)\n", inet_ntoa(clientaddr.sin_addr), ntohs(clientaddr.sin_port));
		if (Fork() == 0) { //connfd�� ó���� ���ο� ���μ���(�ڽ� ���μ���)�� ���� ���� ����
			Close(listenfd); //����� ���μ��������� �� �̻� listenfd�� ���� ������ �ݾ��ְ�

			echo(connfd); //���ϴ� ���� ���� ����
			Close(connfd); //������ ���´�

			printf("Connection (%s, %d) closed\n", inet_ntoa(clientaddr.sin_addr), ntohs(clientaddr.sin_port));
			exit(0);
		}
		Close(connfd); //�θ� ���μ��� �������� client���� ������ ���´�
	}

	exit(0);
}