#include "echo.h"
#include<errno.h>
#include<arpa/inet.h> //htons�� inet_addr�� ���� �Լ��� ����ϱ� ���� ���̺귯��

int main(int argc, char** argv) {
	int clientfd; //client-side���� ��ſ� ���Ǵ� ����
	struct sockaddr_in serveraddr; //client-side������ server�� IP address, port�� ������ ������ ����� ���´�

	char buf[MAXLINE]; //server�� ���� ���ڿ��� ǥ�� �Է¿��� �޾� �����ϰ� �ִ� ����
	
	//1. Socket �Լ��� �̿��Ͽ� clientfd�� ���� ���� ��ũ���ͷ� �����
	clientfd = Socket(AF_INET, SOCK_STREAM, 0); //domain, type, protocol(always 0)

	//2. �Է¹��� ������ IP address�� port�� ������ ��ȯ�Ͽ� serveraddr�� �����Ѵ�
	serveraddr.sin_family = AF_INET;
	serveraddr.sin_addr.s_addr = inet_addr(argv[1]); //inet_addr�� 127.0.0.1�� ���� IP address�� ������ �������ִ� �Լ�
	serveraddr.sin_port = htons(atoi(argv[2])); //���� ��ɾ� �ڿ� 2��° ���ڿ� �� port number�� Big Endian���� �о��

	//3. Connect �Լ��� �̿��Ͽ� ������ ���� ��û�� ������(=connection request)
	Connect(clientfd, (SA*) &serveraddr, sizeof(serveraddr));


	//------------------�������ʹ� ������ ����� �����̴�--------------//


	while (Fgets(buf, MAXLINE, stdin) != NULL) { //1. Terminal�� �� ǥ�� �Է��� buf�� ����

		//2. Write �Լ��� �̿��Ͽ� Terminal input�� server�� ������
		Write(clientfd, buf, strlen(buf));

		//3. Read �Լ��� �̿��Ͽ� server���� ������ ������ buf�� �о �����Ѵ�
		Read(clientfd, buf, MAXLINE);

		//4. Terminal�� �о�� buf�� ����Ѵ�(= echo)
		Fputs(buf, stdout);
	}

	//5. �Է��� ���̻� ���ؼ� �������� �� �� Close�� ����Ǿ��� clientfd�� �ݾƼ� server�ʿ��� EOF�� �о���̰� �����
	Close(clientfd);

	exit(0);
}