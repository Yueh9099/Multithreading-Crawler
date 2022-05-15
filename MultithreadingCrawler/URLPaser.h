
/*
Name: Yue HU
Class: CSCE612
Semester: Spring 2022
*/
#pragma once
#include "pch.h"
class URLPaser {
public:
	const char* url;
	char* modifiableUrl;
	char* query;
	char* path;
	char* request;
	char* host;
	const char* scheme;
	bool modified;
	int port;
	int valid_number;


	

	URLPaser(const char* url);
	

	void parse();
	//truncate the fragment at the end of modifiableUrl.
	void truncateFrag();
	//truncate the ?query at the end of modifiableUrl, and store it in the query.
	void truncateQuery();
	//truncate the /path at the end of modifiableUrl, and store it in the query.
	void truncatePath();
	//remove http:// from the header of the modifiableUrl, and store it in scheme.
	//if scheme invalid, set valid_number as -1
	void truncateHTTP();
	//store the host and port, and truncate at port at the end of modifiableUrl if applicable.
	//if port numbe invalid, set valid_number as -2
	void findPortHost();

	void releaseMeme();
};