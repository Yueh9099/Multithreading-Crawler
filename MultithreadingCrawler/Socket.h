
/*
Name: Yue HU
Class: CSCE612
Semester: Spring 2022
*/
#pragma once
#include "pch.h"

class Socket {
public:
	SOCKET sock;
	char* buf;
	int allocatedSize;
	int curPos;
	bool sockerGenrated;

	struct hostent* remote;
	struct sockaddr_in server;
	Socket();
	
	bool Send(const char* requestType, char* requestIn, char* host, int port);
	bool Read(bool isRobot);
	bool DNS(char* host, int port,bool isPrint);
	bool SendRobotsRequest(char* reqeustType, char* host, int port);
	bool SendRequest(char* request, int reqLen);
	bool Connect();
	void resetBuffer();
	
};