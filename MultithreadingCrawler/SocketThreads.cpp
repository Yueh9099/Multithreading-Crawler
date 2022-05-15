/*
Name: Yue HU
Class: CSCE612
Semester: Spring 2022
*/
#pragma once
#include "pch.h"
#include "SocketThreads.h"
#include "HTMLParserBase.h"

#pragma comment(lib, "ws2_32.lib")

//INITIAL_BUF_SIZE 8KB
const int INITIAL_BUF_SIZE = 2 << 12;
//ROBOT_BUF_MAX 16KB
const int ROBOT_BUF_MAX = 2 << 13;
//PAGE_BUF_MAX 2MB
const int PAGE_BUF_MAX = 2 << 20;

SocketThreads::SocketThreads() {
	buf = new char[INITIAL_BUF_SIZE];
	allocatedSize = INITIAL_BUF_SIZE;
	curPos = 0;
}




bool SocketThreads::init() {
	sock = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
	if (sock == INVALID_SOCKET)
	{
		//printf("socket() generated error %d\n", WSAGetLastError());
		return false;
	}
	return true;
}

void SocketThreads::resetBuf() {
	delete[] buf;
	buf = new char[INITIAL_BUF_SIZE];
	allocatedSize = INITIAL_BUF_SIZE;
	curPos = 0;
}

bool SocketThreads::reset() {
	delete[] buf;
	if (closesocket(sock) == SOCKET_ERROR) {
		return false;
	}

	buf = new char[INITIAL_BUF_SIZE];
	allocatedSize = INITIAL_BUF_SIZE;
	curPos = 0;
	sock = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
	if (sock == INVALID_SOCKET)
	{
		return false;
	}

	return true;
}

bool SocketThreads::close() {
	if (closesocket(sock) == SOCKET_ERROR) {
		return false;
	}
	delete[] buf;
	buf = nullptr;
	return true;
}

bool SocketThreads::DNS(const char* host, int port) {
	if (strlen(host) > MAX_HOST_LEN) {
		//printf("Host exceeds the upper limits");
		return false;
	}
	DWORD IP = inet_addr(host);
	if (IP == INADDR_NONE)
	{
		// if not a valid IP, then do a DNS lookup
		if ((remote = gethostbyname(host)) == NULL)
		{
			//printf("Invalid string: neither FQDN, nor IP address\n");
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
	// setup the port # and protocol type
	server.sin_family = AF_INET;
	server.sin_port = htons(port);
	return true;
}


bool SocketThreads::Connect() {
	// connect to the server on port
	if (connect(sock, (struct sockaddr*)&server, sizeof(struct sockaddr_in)) == SOCKET_ERROR)
	{
		//printf("Connection error: %d\n", WSAGetLastError());
		return false;
	}
	return true;
}

bool SocketThreads::SendRequest(const char* request, int reqLen) {

	if (send(sock, request, reqLen, 0) == SOCKET_ERROR) {
		//printf("Send error: %d\n", WSAGetLastError());
		return false;
	}
	return true;
}

bool SocketThreads::Read(bool isRobot) {
	timeval timeout;
	timeout.tv_sec = 10;
	timeout.tv_usec = 0;
	fd_set readfds;
	int ret;
	while (true) {
		FD_ZERO(&readfds);
		FD_SET(sock, &readfds);

		clock_t time = clock();
		if ((ret = select(0, &readfds, NULL, NULL, &timeout)) > 0) {
			
			int bytes = recv(sock, buf + curPos, allocatedSize - curPos, 0);
			
			time = clock() - time;
			timeout.tv_sec -= ((double)time) / CLOCKS_PER_SEC;

			if (bytes == SOCKET_ERROR) {
				//printf("failed with %d on recv\n", WSAGetLastError());
				break;
			}

			if (bytes == 0) {
				buf[curPos] = '\0';
				return true;
			}

			curPos += bytes;

			if (curPos >= ROBOT_BUF_MAX && isRobot) {
				//printf("Read robots failed with full buffer\n");
				break;
			}
			else if (curPos >= PAGE_BUF_MAX) {
				//printf("Read pages failed with full buffer\n");
				break;
			}

			if (allocatedSize - curPos < (allocatedSize / 8)) {
				char* temp = buf;
				allocatedSize *= 2;
				buf = new char[allocatedSize];
				memcpy(buf, temp, allocatedSize / 2);
				delete[] temp;
			}
		}
		else if (ret == 0) {
			//printf("failed with timed out\n");
			break;
		}
		else {
			//printf("failed with %d on select\n", WSAGetLastError());
			break;
		}
	}
	return false;
}