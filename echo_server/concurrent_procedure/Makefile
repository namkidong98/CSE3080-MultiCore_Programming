CC = gcc
LDLIBS = -lpthread

all : echo_client echo_server

echo_client: echo_client.c echo.h
echo_server: echo_server.c echo.h

clean:
	rm -rf echo_client echo_server
