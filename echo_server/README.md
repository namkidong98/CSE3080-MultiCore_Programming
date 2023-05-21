## base: server와 client 사이의 연결을 구현(concurrent는 불가능)
#### 실행 방식: (server-side) ./echo_server 127.0.0.1 #port     (client-side) ./echo_client 127.0.0.1 #port   
#### 주의사항: concurrent가 불가능하기 때문에 다른 client의 connection request는 기존에 연결된 client와의 연결이 끊어질 때까지 pending된다
![base.png](https://github.com/namkidong98/CSE3080-MultiCore_Programming/blob/main/echo_server/base/base.PNG)

<br/>

## concurrent_procedure: procedure-based approach로 concurrent한 server 구현
#### 구현방식: connection request에 대해 fork로 child process를 만들어서 연결하는 방식으로 concurrent를 구현
#### 주의사항: child process과 연결이 끊어지고 종료된 후 SIGCHLD handler를 통해 zombie process를 reaping 해주어야 한다
#### 실행 방식: (server-side) ./echo_server #port(IP address는 127.0.0.1로 자동 설정)     (client-side) ./echo_client 127.0.0.1 #port   
#### 단점: additional overhead for process control, hard to share data between processes
![concurrent_procedure.png](https://github.com/namkidong98/CSE3080-MultiCore_Programming/blob/main/echo_server/concurrent_procedure/concurrent_procedure.PNG)

<br/>

## concurrent_event: event-based approach로 concurrent한 server 구현
#### 구현방식: select함수를 통해 이벤트가 발생한 디스크립터를 찾아서 연결
#### 실행 방식: (server-side) ./echo_server #port     (client-side) ./echo_client 127.0.0.1 #port   
#### 문제점: blocking problem(server가 clinet랑 연결되면 종료 전까지 대기하는 문제가 발생 --> concurrent가 제대로 실현되지 못함)
![concurrent_event.png](https://github.com/namkidong98/CSE3080-MultiCore_Programming/blob/main/echo_server/concurrent_event/concurrent_event.PNG)

<br/>

## concurrent_event_finer-granualarity: event-based approach에서 blocking problem을 해소하고자 fined grained 방식이 추가
#### 구현방식: client와 연결되는 connfd의 array를 만들고 select 함수를 통해 event가 발생한 것에 대해 처리하는 방식으로 구현
#### 실행 방식: (server-side) ./echo_server #port     (client-side) ./echo_client 127.0.0.1 #port   
#### 장점: no process or thread control overhead
#### 단점: more complex to code, hard to provide fine-grained concurrency
![concurrent_event_fined.png](https://github.com/namkidong98/CSE3080-MultiCore_Programming/blob/main/echo_server/concurrent_event_finer-granualarity/concurrent_event_fined.PNG)

<br/>

## concurrent_thread: thread_based approach로 concurrent한 server 구현
#### 구현방식: 
#### 실행 방식: (server-side) ./echo_server #port     (client-side) ./echo_client 127.0.0.1 #port   
#### 장점: easy to share data between threads, threads are more efficient than processes
#### 단점: difficult to debug

<br/>
