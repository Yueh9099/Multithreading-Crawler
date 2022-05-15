/*
Name: Yue HU
Class: CSCE612
Semester: Spring 2022
*/

#include "pch.h"
#include "Socket.h"
#include "HTMLParserBase.h"

#pragma comment(lib, "ws2_32.lib")


//INITIAL_BUF_SIZE 8KB
const int INITIAL_BUF_SIZE = 2 << 12;
//ROBOT_BUF_MAX 16KB
const int ROBOT_BUF_MAX = 2<<13;
//PAGE_BUF_MAX 2MB
const int PAGE_BUF_MAX = 2 << 20;

Socket::Socket() {
	buf = new char[INITIAL_BUF_SIZE];
	allocatedSize = INITIAL_BUF_SIZE;
	curPos = 0;
	sockerGenrated = true;
	

	//Initialize WinSock
	WSADATA wsaData;
	WORD wVersionRequested = MAKEWORD(2, 2);
	if (WSAStartup(wVersionRequested, &wsaData) != 0) {
		printf("WSAStartup error %d\n", WSAGetLastError());
		WSACleanup();
		sockerGenrated = false;
		return;
	}

	// open a TCP socket
	sock = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
	if (sock == INVALID_SOCKET)
	{
		printf("socket() generated error %d\n", WSAGetLastError());
		WSACleanup();
		sockerGenrated = false;
		return;
	}
}

void Socket::resetBuffer() {
	delete[] buf;
	buf = new char[INITIAL_BUF_SIZE];
	allocatedSize = INITIAL_BUF_SIZE;
	curPos = 0;
}


bool Socket::DNS(char* host, int port, bool isPrint) {
	if (strlen(host) > MAX_HOST_LEN) {
		printf("Host exceeds the upper limits");
		return false;
	}

	// first assume that the host is an IP address
	if (isPrint) {
		printf("\tDoing DNS... ");
	}
	
	clock_t t = clock();
	DWORD IP = inet_addr(host);
	if (IP == INADDR_NONE)
	{
		// if not a valid IP, then do a DNS lookup
		if ((remote = gethostbyname(host)) == NULL)
		{
			printf("Invalid string: neither FQDN, nor IP address\n");
			return false;
		}
		else // take the first IP address and copy into sin_addr
			memcpy((char*)&(server.sin_addr), remote->h_addr, remote->h_length);
	}
	else
	{
		// if a valid IP, directly drop its binary version into sin_addr
		server.sin_addr.S_un.S_addr = IP;
	}

	t = clock() - t;
	if (isPrint) {
		printf("done in %.2f ms, found %s\n", ((double)t) / CLOCKS_PER_SEC * 1000, inet_ntoa(server.sin_addr));
	}
	

	// setup the port # and protocol type
	server.sin_family = AF_INET;
	server.sin_port = htons(port);
	
	return true;
}

bool Socket::SendRequest(char* request, int reqLen) {
	
	if (send(sock, request, reqLen, 0) == SOCKET_ERROR) {
		printf("Send error: %d\n", WSAGetLastError());
		return false;
	}
	
	return true;

}

// This is used after DNS()
bool Socket::SendRobotsRequest(char* requestType, char* host, int port) {
	//calculate request length, first line length
	int requestLen = strlen(requestType) + 1 + strlen("/robots.txt") + 1 + 8 + 2; //1 is from space, 8 is from HTTP/1.0, 2 from \r\n
	// add second line length
	requestLen += strlen("User-agent: myTAMUcrawler/1.0") + 2;
	// add third line length
	requestLen += 5 + 1 + strlen(host) + 2;// 5 is from Host:
	// add forth line length
	requestLen += strlen("Connection: close") + 2 + 2 + 1; // 1 is for NULL

	server.sin_port = htons(port);
	printf("\t\b\b* Connecting on on robots...");
	clock_t t = clock();
	// connect to the server on port
	if (connect(sock, (struct sockaddr*)&server, sizeof(struct sockaddr_in)) == SOCKET_ERROR)
	{
		printf("Connection error: %d\n", WSAGetLastError());
		return false;
	}

	t = clock() - t;
	printf("done in %.2f ms\n", ((double)t) / CLOCKS_PER_SEC * 1000);

	//construct sendBuf

	char* sendBuf = new char[requestLen];
	strcpy(sendBuf, requestType);
	strcat(sendBuf, " ");
	strcat(sendBuf, "/robots.txt");
	strcat(sendBuf, " HTTP/1.0\r\nUser-agent: myTAMUcrawler/1.0\r\n");
	strcat(sendBuf, "Host: ");
	strcat(sendBuf, host);
	strcat(sendBuf, "\r\nConnection: Close\r\n\r\n");

	//printf("%s", sendBuf);// for debug use

	if (send(sock, sendBuf, requestLen - 1, 0) == SOCKET_ERROR) {
		printf("Send error: %d\n", WSAGetLastError());
		return false;
	}
	return true;
}

bool Socket::Connect() {
	//printf("\t\b\b* Connecting on page...");
	clock_t t = clock();
	// connect to the server on port
	if (connect(sock, (struct sockaddr*)&server, sizeof(struct sockaddr_in)) == SOCKET_ERROR)
	{
		printf("Connection error: %d\n", WSAGetLastError());
		return false;
	}

	//t = clock() - t;
	printf("done in %.2f ms\n", ((double)t) / CLOCKS_PER_SEC * 1000);
	return true;
}


bool Socket::Send(const char* requestType, char* requestIn, char* host, int port ) {



	//calculate request length, first line length
	int requestLen = strlen(requestType) + 1 + strlen(requestIn) + 1 + 8 + 2; //1 is from space, 8 is from HTTP/1.0, 2 from \r\n
	// add second line length
	requestLen += strlen("User-agent: myTAMUcrawler/1.0") + 2;
	// add third line length
	requestLen += 5 + 1 + strlen(host) + 2;// 5 is from Host:
	// add forth line length
	requestLen += strlen("Connection: close") + 2 + 2 + 1; // 1 is for NULL

	if (requestLen > MAX_REQUEST_LEN) {
		printf("Requst exceeds the upper limits");
		return false;
	}
	/*
	if (!DNS(host)) {
		return false;
	}
	server.sin_port = htons(port);		// host-to-network flips the byte order
	*/
	

	//Connect();

	//construct sendBuf

	char* sendBuf = new char[requestLen];

	strcpy(sendBuf, requestType);
	strcat(sendBuf, " ");
	strcat(sendBuf, requestIn);
	strcat(sendBuf, " HTTP/1.0\r\nUser-agent: myTAMUcrawler/1.0\r\n");
	strcat(sendBuf, "Host: ");
	strcat(sendBuf, host);
	strcat(sendBuf, "\r\nConnection: Close\r\n\r\n");

	//printf("%s", sendBuf);// for debug use

	if (send(sock, sendBuf, requestLen - 1, 0) == SOCKET_ERROR) {
		printf("\tSend error: %d\n", WSAGetLastError());
		return false;
	}

	return true;

}



bool Socket::Read(bool isRobot) {

	// this comes from https://stackoverflow.com/questions/4181784/how-to-set-socket-timeout-in-c-when-making-multiple-connections
	//set timeout to 10 seconds
	timeval timeout;
	timeout.tv_sec = 10;
	timeout.tv_usec = 0;

	fd_set readfds;
	int ret;


	while (true) {
		// source: https://stackoverflow.com/questions/4301760/winsock-selecting-runtime-error
		FD_ZERO(&readfds);
		FD_SET(sock, &readfds);

		clock_t time = clock();
		if ((ret = select(0, &readfds, NULL, NULL,&timeout)) > 0) {
			
			int bytes = recv(sock, buf+curPos, allocatedSize - curPos,0);
			time = clock() - time;
			timeout.tv_sec -= ((double)time) / CLOCKS_PER_SEC;

			if (bytes == SOCKET_ERROR) {
				printf("failed with %d on recv\n", WSAGetLastError());
				break;
			}

			if (bytes == 0) {
				buf[curPos] = '\0';

				return true;
			}

			curPos += bytes;

			if (curPos >= ROBOT_BUF_MAX && isRobot) {
				printf("Read robots failed with full buffer\n");
				break;
			}
			else if (curPos >= PAGE_BUF_MAX) {
				printf("Read pages failed with full buffer\n");
				break;
			}

			

			if (allocatedSize - curPos < (allocatedSize/8)) {
				char* temp = buf;
				allocatedSize *= 2;
				buf = new char[allocatedSize];
				memcpy(buf, temp, allocatedSize/2);
				delete[] temp;
			}
		}
		else if (ret == 0) {
			printf("failed with timed out\n");
			break;
		}
		else {
			printf("failed with %d on select\n", WSAGetLastError());
			break;
		}
	}
	return false;
}