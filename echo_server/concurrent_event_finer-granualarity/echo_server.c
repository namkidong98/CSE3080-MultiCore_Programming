#include "echo.h"
#include<errno.h>
#include<arpa/inet.h> //htons�� inet_addr�� ���� �Լ��� ����ϱ� ���� ���̺귯��
#include <sys/select.h> //select�Լ��� fdset�� ���� �Լ���

typedef struct {
	int maxfd;						//read_set���� ���� ū ���� ��ũ����
	fd_set read_set;				//set of all active descriptor
	fd_set ready_set;				//subset of desccriptors ready for reading (�б� �̺�Ʈ�� �߻��� 
	int nready;						//select �Լ��� ���� �ľ���, �̺�Ʈ�� �߻��� ��ũ������ ����
	int maxi;						//client array�� top index
	int clientfd[FD_SETSIZE];		//set of active active desriptors(����Ǿ� �ִ� client���� ���� ��ũ���� ����)
	rio_t clientrio[FD_SETSIZE];	//set of active read buffer(�� client�� �Ҵ�Ǿ� �ִ� �б�� ����)
}pool;

void init_pool(int listenfd, pool* p);
void add_client(int connfd, pool* p);
void check_client(pool* p);

int byte_cnt = 0;	//�������� �޾ƿ� �������� ��ü ����Ʈ ���� �����ϴ� ����

int main(int argc, char** argv) {
	int listenfd; //����� ��û ���� �� ���Ǵ� ����(ó�� connection request���� ���)
	int connfd; //client�� ����� �� ���� ������ �ְ� ���� �� ���Ǵ� ����(connected descriptor)
	struct sockaddr_in clientaddr; //client�� IP address�� port�� ������ �޾ƿ��� ���� ����
	socklen_t clientlen = sizeof(clientaddr);
	static pool pool;

	//�Է� ���� ó��
	if (argc != 2) { //IP address�� �⺻������ ���� ȣ��Ʈ�� ����Ű���� �����Ǿ� �ִ�
		fprintf(stderr, "usage: %s <port>\n", argv[0]);
		exit(0);
	}

	listenfd = Open_listenfd(argv[1]);
	//Getaddrinfo(NULL, port, &hints, &listp); - ù��° ���ڿ� hostname or IP address�� ����Ű�� ���ڿ��� �´�
											// - NULL�� ���� ���� ȣ��Ʈ(���� �ý���, 127.0.0.1)�� ����Ų��
	init_pool(listenfd, &pool); //pool�� �ʱ�ȭ

	while (1) {
		pool.ready_set = pool.read_set;
		pool.nready = Select(pool.maxfd + 1, &pool.ready_set, NULL, NULL, NULL); //����� ������ ��ũ������ ������ ��ȯ, ������ ��� -1�� ��ȯ
		//ù ��° ����: ����͸��� ��ũ������ ����			�� ��° ����: �б� �̺�Ʈ�� �߻��� ��ũ���� ����
		//�� ��° ����: ���� �̺�Ʈ�� �߻��� ��ũ���� ����	�� ��° ����: ���� �̺�Ʈ�� �߻��� ��ũ���� ����
		//�ټ� ��° ����: timeout�� ������ �ð����� ���(blocking�� ����, NULL�̸� ������ blocking)
		
		
		if (FD_ISSET(listenfd, &pool.ready_set)) {	//select�� �б� �̺�Ʈ�� �߻��� ��ũ���� ����(ready_set)�� ������ ��, �ű⿡ listenfd�� �ִٸ�
			connfd = Accept(listenfd, (SA*)&clientaddr, &clientlen);
			printf("Connection from (%s, %d)\n", inet_ntoa(clientaddr.sin_addr), ntohs(clientaddr.sin_port));

			add_client(connfd, &pool); //connection request�� �� client�� client array�� �߰�
		}
		
		check_client(&pool); //���� while loop ������ client array�� ���� �̺�Ʈ�� �߻��� �Ϳ� ���� ó��
	}

	exit(0);
}

void init_pool(int listenfd, pool* p) { //pool�� �ʱ�ȭ�ϴ� �Լ�(�ʱ⿡ ����� ��ũ���ʹ� ����)
	int i;
	p->maxi = -1;
	for (i = 0; i < FD_SETSIZE; i++) {
		p->clientfd[i] = -1; //������� �ʴ� client ���� ��ũ���ʹ� -1�� ǥ��(�ʱ�ȭ)
	}

	p->maxfd = listenfd;			//�����ϰ� �߰��� fd�� listenfd�̴� maxfd�� listenfd(3)�� ����( maxfd = 3 )
	FD_ZERO(&p->read_set);			//pool�� read_set�� �ʱ�ȭ
	FD_SET(listenfd, &p->read_set); //pool�� read_set�� listenfd�� �߰�
}

void add_client(int connfd, pool* p) {
	int i;
	p->nready--;
	for (i = 0; i < FD_SETSIZE; i++) { //client�� ����� connfd�� ������ �� ������ ã�� �۾�
		if (p->clientfd[i] < 0) { //�� ���� ������ -1�� �ʱ�ȭ �Ǿ� ����
			p->clientfd[i] = connfd; //�� ���� ���Կ� connfd�� �ְ�
			Rio_readinitb(&p->clientrio[i], connfd); //�б� ���� rio�� �ʱ�ȭ

			FD_SET(connfd, &p->read_set); //read_set�� connfd �߰�

			if (connfd > p->maxfd) p->maxfd = connfd; //maxfd���� ũ�� ����
			if (i > p->maxi) p->maxi = i;			  //maxi���� ũ�� ����
			
			break; //client�� ����� connfd�� �� ���Կ� �־����� for loop ����
		}
	}

	if (i == FD_SETSIZE) //�� ������ ã�� ���� i�� FD_SETEIZE�� �Ǿ� for loop�� ����Ǿ��ٸ�
		app_error("add_client error: Too many clients"); //�� ������ ���ٴ� ���� ǥ��
}

void check_client(pool* p) {
	int i, connfd, n;
	char buf[MAXLINE];
	rio_t rio;

	//clientfd�� array�� ���鼭 �̺�Ʈ�� �߻��ߴ��� �ľ��ϰ� echo ����� ����, ���� EOF�� �Դٸ�(���� ���� signal) clientfd���� �����ϴ� �۾�
	for (i = 0; (i <= p->maxi) && (p->nready > 0); i++) { //i �ε����� max���� �۰� ���� �о�� �� ��ũ���Ͱ� �ִٸ�
		connfd = p->clientfd[i];
		rio = p->clientrio[i];

		if ( (connfd > 0) && FD_ISSET(connfd, &p->ready_set) ) { //connfd�� ����Ǿ� �ִ� ���� ��ũ�����̰�, ready_set�� �־ �̺�Ʈ�� �߻��� ��Ȳ�̸�
			p->nready--;

			if ((n = Rio_readlineb(&rio, buf, MAXLINE)) != 0) { //�Է��� ������
				byte_cnt += n;
				printf("Server received %d (%d total) bytes on fd %d\n", n, byte_cnt, connfd);
				Rio_writen(connfd, buf, n); //WRITE to client (= echo)
			}

			else { //EOF�� �Դٸ�
				Close(connfd);	//����� ��ũ���͸� �ݰ�
				printf("Connection on fd %d closed\n", connfd);

				FD_CLR(connfd, &p->read_set); //�ش� ���� ��ũ���͸� �о�� �� ��ũ���� ���տ��� ����
				p->clientfd[i] = -1; //�ش� client�� ���� ��ũ���͸� �����ϴ� ���� -1�� �ٲپ� �� ������ �ǹ��ϰ� �Ѵ�
			}
		}
	}
}