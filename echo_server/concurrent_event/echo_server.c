#include "echo.h"
#include<errno.h>
#include<arpa/inet.h> //htons�� inet_addr�� ���� �Լ��� ����ϱ� ���� ���̺귯��
#include <sys/select.h> //select�Լ��� fdset�� ���� �Լ���

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

void command() {
	char buf[MAXLINE];
	if (!Fgets(buf, MAXLINE, stdin)) exit(0); //terminal���� �Է¹��� ���� ������ ����(����)
	printf("%s", buf);	//terminal���� �Է¹��� ���� �״�� ���
}

int main(int argc, char** argv) {
	int listenfd; //����� ��û ���� �� ���Ǵ� ����(ó�� connection request���� ���)
	int connfd; //client�� ����� �� ���� ������ �ְ� ���� �� ���Ǵ� ����(connected descriptor)
	struct sockaddr_in clientaddr; //client�� IP address�� port�� ������ �޾ƿ��� ���� ����
	socklen_t clientlen = sizeof(clientaddr);

	fd_set read_set, ready_set;

	//�Է� ���� ó��
	if (argc != 2) { //IP address�� �⺻������ ���� ȣ��Ʈ�� ����Ű���� �����Ǿ� �ִ�
		fprintf(stderr, "usage: %s <port>\n", argv[0]);
		exit(0);
	}

	listenfd = Open_listenfd(argv[1]);
	//Getaddrinfo(NULL, port, &hints, &listp); - ù��° ���ڿ� hostname or IP address�� ����Ű�� ���ڿ��� �´�
											// - NULL�� ���� ���� ȣ��Ʈ(���� �ý���, 127.0.0.1)�� ����Ų��

	FD_ZERO(&read_set);					//read_set�� �ʱ�ȭ
	FD_SET(STDIN_FILENO, &read_set);	//stdin(terminal input)�� read_set�� �߰�
	FD_SET(listenfd, &read_set);		//listenfd�� read_set�� �߰�

	while (1) {
		ready_set = read_set;			//select�Լ��� ���ڷ� ���� fdset�� �����Ű�� ������ read_set���� �ٸ� fdset�� �ʿ�
		
		Select(listenfd + 1, &ready_set, NULL, NULL, NULL);		//����� ������ ��ũ������ ������ ��ȯ, ������ ��� -1�� ��ȯ
		//ù ��° ����: ����͸��� ��ũ������ ����			�� ��° ����: �б� �̺�Ʈ�� �߻��� ��ũ���� ����
		//�� ��° ����: ���� �̺�Ʈ�� �߻��� ��ũ���� ����	�� ��° ����: ���� �̺�Ʈ�� �߻��� ��ũ���� ����
		//�ټ� ��° ����: timeout�� ������ �ð����� ���(blocking�� ����, NULL�̸� ������ blocking)
		
		if (FD_ISSET(STDIN_FILENO, &ready_set)) //select�� �б� �̺�Ʈ�� �߻��� ��ũ���� ����(ready_set)�� ������ ��, �ű⿡ STDIN�� �ִٸ�
			command();
		if (FD_ISSET(listenfd, &ready_set)) {	//select�� �б� �̺�Ʈ�� �߻��� ��ũ���� ����(ready_set)�� ������ ��, �ű⿡ listenfd�� �ִٸ�
			connfd = Accept(listenfd, (SA*)&clientaddr, &clientlen);
			printf("Connection from (%s, %d)\n", inet_ntoa(clientaddr.sin_addr), ntohs(clientaddr.sin_port));

			echo(connfd); //���ϴ� ���� ���� ����
			Close(connfd); //������ ���´�

			printf("Connection (%s, %d) closed\n", inet_ntoa(clientaddr.sin_addr), ntohs(clientaddr.sin_port));
		}
		
	}

	exit(0);
}