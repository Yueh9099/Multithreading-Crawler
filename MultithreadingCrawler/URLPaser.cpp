/*
Name: Yue HU
Class: CSCE612
Semester: Spring 2022
*/

#include "pch.h"
#include "URLPaser.h"



URLPaser::URLPaser(const char* url) {
	this->url = url;
	modified = false;
	modifiableUrl = new char[strlen(url)+1];
	strcpy(modifiableUrl, url);
	query = nullptr;
	path = nullptr;
	request = nullptr;
	host = nullptr;
	scheme = "http://";
	port = 80;
	valid_number = 0;
}


void URLPaser::releaseMeme() {
	if (modified) {
		modifiableUrl -= 7;
		
	}
	
	delete[] modifiableUrl;
	modifiableUrl = nullptr;
	
	delete[] query;
	query = nullptr;
	delete[] path;
	path = nullptr;
	delete[] request;
	request = nullptr;
	delete[] host;
	host = nullptr;
	
}





void URLPaser::parse() {
	truncateHTTP();

	// invalid scheme
	if (valid_number == -1) {
		return;
	}

	truncateFrag();
	truncateQuery();
	truncatePath();
	findPortHost();

	//invalide port number
	if (valid_number == -2) {
		return;
	}

	//compose the request, if query is empty
	if (query == nullptr) {
		request = new char[strlen(path)+1];
		strcpy(request, path);
		return;

	}
	request = new char[strlen(path) + strlen(query) + 1];
	strcpy(request, path);
	
	strcat(request, query);
};



void URLPaser::truncateFrag() {
	char* fragment = strchr(modifiableUrl, '#');
	if (fragment == NULL) {
		return;
	}
	int lenFrag = strlen(fragment);
	modifiableUrl[strlen(modifiableUrl) - lenFrag] = '\0';
};

void URLPaser::truncateQuery() {
	char* foundQuery = strchr(modifiableUrl, '?');
	if (foundQuery == NULL) {
		return;
	}
	int lenQuery = strlen(foundQuery);
	query = new char[lenQuery+1];
	strcpy(query, foundQuery);
	modifiableUrl[strlen(modifiableUrl) - lenQuery] = '\0';
}

void URLPaser::truncatePath() {
	char* foundPath = strchr(modifiableUrl, '/');

	if (foundPath == NULL) {
		path = new char[2];
		strcpy(path, "/");
		return;
	}
	int lenPath = strlen(foundPath);
	path = new char[lenPath+1];
	strcpy(path, foundPath);
	modifiableUrl[strlen(modifiableUrl) - lenPath] = '\0';
}

void URLPaser::truncateHTTP() {
	for (int i = 0; i < 7; i++) {
		if (scheme[i] != modifiableUrl[i]) {
			valid_number = -1;
			return;
		}
	}
	modifiableUrl = modifiableUrl + 7;
	modified = true;
}

void URLPaser::findPortHost() {
	char* foundPort = strchr(modifiableUrl, ':');

	//if port not found, set host and return
	if (foundPort == NULL) {
		host = new char[strlen(modifiableUrl)+1];
		strcpy(host, modifiableUrl);
		return;
	}

	foundPort++;
	//if port is empty, invalid and return
	if (*foundPort == '\0') {
		valid_number = -2;
		return;

	}

	//port not empty, check if port is a number
	int lenPort = strlen(foundPort);
	//if invalid, return
	for (int i = 0; i < lenPort; i++) {
		if (isdigit(foundPort[i]) == 0) {
			valid_number = -2;
			return;
		}
	}
	//port is a non-negative int number, check if it is out of range
	int temp = atoi(foundPort);
	if (temp > 0 && temp <= 65535) {
		port = temp;
	}
	else {
		valid_number = -2;
		return;
	}

	
	modifiableUrl[strlen(modifiableUrl) - lenPort-1] = '\0';
	host = new char[strlen(modifiableUrl)+1];
	strcpy(host, modifiableUrl);
	
}