
/*
Name: Yue HU
Class: CSCE612
Semester: Spring 2022
*/
#pragma once
#include "pch.h"

class SocketThreads {
public:
	SOCKET sock;
	char* buf;
	int allocatedSize;
	int curPos;
	

	struct hostent* remote;
	struct sockaddr_in server;
	SocketThreads();
	//~SocketThreads();
	
	bool Read(bool isRobot);
	bool DNS(const char* host, int port);
	bool init();
	bool SendRequest(const char* request, int reqLen);
	bool Connect();
	void resetBuf();
	bool reset();
	bool close();
};