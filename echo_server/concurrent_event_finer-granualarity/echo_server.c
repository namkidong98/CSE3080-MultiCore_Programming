#include "echo.h"
#include<errno.h>
#include<arpa/inet.h> //htons나 inet_addr과 같은 함수를 사용하기 위한 라이브러리
#include <sys/select.h> //select함수나 fdset에 관한 함수들

typedef struct {
	int maxfd;						//read_set에서 제일 큰 파일 디스크립터
	fd_set read_set;				//set of all active descriptor
	fd_set ready_set;				//subset of desccriptors ready for reading (읽기 이벤트가 발생된 
	int nready;						//select 함수를 통해 파악한, 이벤트가 발생한 디스크립터의 개수
	int maxi;						//client array의 top index
	int clientfd[FD_SETSIZE];		//set of active active desriptors(연결되어 있는 client들의 파일 디스크립터 모음)
	rio_t clientrio[FD_SETSIZE];	//set of active read buffer(각 client에 할당되어 있는 읽기용 버퍼)
}pool;

void init_pool(int listenfd, pool* p);
void add_client(int connfd, pool* p);
void check_client(pool* p);

int byte_cnt = 0;	//서버에서 받아온 데이터의 전체 바이트 수를 저장하는 변수

int main(int argc, char** argv) {
	int listenfd; //통신을 요청 받을 때 사용되는 소켓(처음 connection request에만 사용)
	int connfd; //client랑 연결된 후 직접 정보를 주고 받을 때 사용되는 소켓(connected descriptor)
	struct sockaddr_in clientaddr; //client의 IP address와 port의 정보를 받아오기 위한 변수
	socklen_t clientlen = sizeof(clientaddr);
	static pool pool;

	//입력 예외 처리
	if (argc != 2) { //IP address는 기본적으로 로컬 호스트를 가리키도록 설정되어 있다
		fprintf(stderr, "usage: %s <port>\n", argv[0]);
		exit(0);
	}

	listenfd = Open_listenfd(argv[1]);
	//Getaddrinfo(NULL, port, &hints, &listp); - 첫번째 인자에 hostname or IP address를 가리키는 문자열이 온다
											// - NULL이 오면 로컬 호스트(현재 시스템, 127.0.0.1)을 가리킨다
	init_pool(listenfd, &pool); //pool을 초기화

	while (1) {
		pool.ready_set = pool.read_set;
		pool.nready = Select(pool.maxfd + 1, &pool.ready_set, NULL, NULL, NULL); //입출력 가능한 디스크립터의 개수를 반환, 에러의 경우 -1을 반환
		//첫 번째 인자: 모니터링할 디스크립터의 개수			두 번째 인자: 읽기 이벤트가 발생한 디스크립터 집합
		//세 번째 인자: 쓰기 이벤트가 발생한 디스크립터 집합	네 번째 인자: 예외 이벤트가 발생한 디스크립터 집합
		//다섯 번째 인자: timeout에 지정된 시간동안 대기(blocking을 통제, NULL이면 무한정 blocking)
		
		
		if (FD_ISSET(listenfd, &pool.ready_set)) {	//select로 읽기 이벤트가 발생한 디스크립터 집합(ready_set)을 갱신한 후, 거기에 listenfd가 있다면
			connfd = Accept(listenfd, (SA*)&clientaddr, &clientlen);
			printf("Connection from (%s, %d)\n", inet_ntoa(clientaddr.sin_addr), ntohs(clientaddr.sin_port));

			add_client(connfd, &pool); //connection request가 온 client를 client array에 추가
		}
		
		check_client(&pool); //다음 while loop 이전에 client array를 보고 이벤트가 발생한 것에 대해 처리
	}

	exit(0);
}

void init_pool(int listenfd, pool* p) { //pool을 초기화하는 함수(초기에 연결된 디스크립터는 없다)
	int i;
	p->maxi = -1;
	for (i = 0; i < FD_SETSIZE; i++) {
		p->clientfd[i] = -1; //사용하지 않는 client 파일 디스크립터는 -1로 표시(초기화)
	}

	p->maxfd = listenfd;			//유일하게 추가된 fd가 listenfd이니 maxfd를 listenfd(3)로 설정( maxfd = 3 )
	FD_ZERO(&p->read_set);			//pool의 read_set을 초기화
	FD_SET(listenfd, &p->read_set); //pool의 read_set에 listenfd를 추가
}

void add_client(int connfd, pool* p) {
	int i;
	p->nready--;
	for (i = 0; i < FD_SETSIZE; i++) { //client와 연결된 connfd를 저장할 빈 슬롯을 찾는 작업
		if (p->clientfd[i] < 0) { //안 쓰는 슬롯은 -1로 초기화 되어 있음
			p->clientfd[i] = connfd; //안 쓰는 슬롯에 connfd를 넣고
			Rio_readinitb(&p->clientrio[i], connfd); //읽기 위해 rio를 초기화

			FD_SET(connfd, &p->read_set); //read_set에 connfd 추가

			if (connfd > p->maxfd) p->maxfd = connfd; //maxfd보다 크면 갱신
			if (i > p->maxi) p->maxi = i;			  //maxi보다 크면 갱신
			
			break; //client와 연결된 connfd를 빈 슬롯에 넣었으니 for loop 종료
		}
	}

	if (i == FD_SETSIZE) //빈 슬롯을 찾지 못해 i가 FD_SETEIZE가 되어 for loop이 종료되었다면
		app_error("add_client error: Too many clients"); //빈 슬롯이 없다는 것을 표현
}

void check_client(pool* p) {
	int i, connfd, n;
	char buf[MAXLINE];
	rio_t rio;

	//clientfd의 array를 돌면서 이벤트가 발생했는지 파악하고 echo 기능을 수행, 만약 EOF가 왔다면(연결 종료 signal) clientfd에서 제거하는 작업
	for (i = 0; (i <= p->maxi) && (p->nready > 0); i++) { //i 인덱스의 max보다 작고 아직 읽어야 할 디스크립터가 있다면
		connfd = p->clientfd[i];
		rio = p->clientrio[i];

		if ( (connfd > 0) && FD_ISSET(connfd, &p->ready_set) ) { //connfd가 연결되어 있는 실제 디스크립터이고, ready_set에 있어서 이벤트가 발생한 상황이면
			p->nready--;

			if ((n = Rio_readlineb(&rio, buf, MAXLINE)) != 0) { //입력이 있으면
				byte_cnt += n;
				printf("Server received %d (%d total) bytes on fd %d\n", n, byte_cnt, connfd);
				Rio_writen(connfd, buf, n); //WRITE to client (= echo)
			}

			else { //EOF가 왔다면
				Close(connfd);	//연결된 디스크립터를 닫고
				printf("Connection on fd %d closed\n", connfd);

				FD_CLR(connfd, &p->read_set); //해당 연결 디스크립터를 읽어야 할 디스크립터 집합에서 제거
				p->clientfd[i] = -1; //해당 client의 파일 디스크립터를 저장하던 것을 -1로 바꾸어 빈 슬롯을 의미하게 한다
			}
		}
	}
}