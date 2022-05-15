#pragma once
#include "pch.h"
using namespace std;

class URLPaserFile {
public:
	vector<string> urls;
	int size;
	//file statu > 0 means the file is opened successfully
	int fileStatus;

	URLPaserFile(char* fileName);

};