#include "pch.h"
#include "URLPaserFile.h"

using namespace std;

URLPaserFile::URLPaserFile(char* fileName) {
	//https://stackoverflow.com/questions/2409504/using-c-filestreams-fstream-how-can-you-determine-the-size-of-a-file
	ifstream file(fileName);
	size = 0;
	string curLine;
	if (!file.is_open()) {
		this->fileStatus = -1;
		return;
	}
	this->fileStatus = 1;
	
	
	while (getline(file, curLine))
	{
		this->urls.push_back(curLine);
		size += strlen(curLine.c_str());
	}
	
}
