// HW1.cpp : This file contains the 'main' function. Program execution begins and ends there.
//

/*
Name: Yue HU
Class: CSCE612
Semester: Spring 2022
*/

#include "pch.h"
#include "URLPaser.h"
#include "Socket.h"
#include "HTMLParserBase.h"
#include "URLPaserFile.h"
#include "SocketThreads.h"

void requestURL(Socket& socket, URLPaser& urlPaser) {
	

	if (socket.Send("GET", urlPaser.request, urlPaser.host, urlPaser.port)) {
		printf("\tLoading... ");
		clock_t t = clock();
		if (socket.Read(false)) {
			closesocket(socket.sock);
			char* header = socket.buf;

			char* page = strstr(header, "\r\n\r\n");
			if (page == NULL) {
				printf("failed with non-HTTP header\n");
				return;
			}
			t = clock() - t;
			printf("done in %.2f ms with %d bytes\n", ((double)t) / CLOCKS_PER_SEC * 1000, socket.curPos);

			page[0] = '\0';
			page += 4;
			printf("\tVerifying header... ");
			//printf(header);//debug use
			char* statusPtr = header + 9;
			int statusCode = atoi(statusPtr);
			printf("status code %d\n", statusCode);
			if (statusCode >= 200 && statusCode < 300) {
				printf("\t\b\b+ Parsing page... ");
				t = clock();
				HTMLParserBase HTMLPaser;
				char* baseURL = new char[strlen(urlPaser.url) + 1];
				strcpy(baseURL, urlPaser.url);
				int* nLinks = new int[1];
				char* bufferParsedHTML = HTMLPaser.Parse(page, strlen(page), baseURL, strlen(baseURL), nLinks);
				t = clock() - t;
				printf("done in %.2f ms with %d links\n", ((double)t) / CLOCKS_PER_SEC * 1000, *nLinks);
			}
			socket.resetBuffer();

			//printf("\n--------------------------------------\n");
			//printf(header);
		}
	}
}

bool parseURL(URLPaser& urlPaser) {
	urlPaser.parse();

	//invalid scheme
	if (urlPaser.valid_number == -1) {
		printf("failed with invalid scheme\n");
		return false;

	}

	//invalid port number
	if (urlPaser.valid_number == -2) {
		printf("failed with invalid port\n");
		return false;
	}
	
	return true;
}

void requestURLWithRobot(Socket& socket, URLPaser& urlPaser) {
	
	if (socket.SendRobotsRequest((char* )"HEAD", urlPaser.host, urlPaser.port)) {
		printf("\tLoading... ");
		clock_t t = clock();
		if (socket.Read(true)){
			closesocket(socket.sock);
			WSACleanup();
			char* header = socket.buf;
			char* page = strstr(header, "\r\n\r\n");
			if (page == NULL) {
				printf("failed with non-HTTP header\n");
				return;
			}
			t = clock() - t;
			printf("done in %.2f ms with %d bytes\n", ((double)t) / CLOCKS_PER_SEC * 1000, socket.curPos);
			page[0] = '\0';
			page += 4;
			printf("\tVerifying header... ");
			//printf(header);//debug use
			char* statusPtr = header + 9;
			int statusCode = atoi(statusPtr);
			printf("status code %d\n", statusCode);
			if (statusCode >= 400) {
				Socket newSocket;
				newSocket.DNS(urlPaser.host, urlPaser.port, false);
				if (!newSocket.Connect()) {
					return;
				}
				
				requestURL(newSocket, urlPaser);
				
				
			}
		}
	}
}

//
//begin the functions used in multiple threads
//

struct Parameters {
	queue<string> urlsQue;
	unordered_set<string> seenHosts;
	unordered_set<DWORD> seenIPs;

	HANDLE urlsQueMutex;
	HANDLE hostSetMutex;
	HANDLE IPSetMutex;
	HANDLE thredsQMutex;
	HANDLE robotMutex;
	HANDLE crawlMute;
	HANDLE downloadMutex;
	HANDLE httpMutex;
	HANDLE tamuMutex;


	int numUniqueHost = 0;
	int numDNSPass = 0;
	int numUniqueIP = 0;
	int numThreadsQ = 0;
	int numExtractedURLs = 0;
	int numRobotPass = 0;
	int numCrawledPass = 0;
	int numBytes = 0;
	int numContainTAMU = 0;
	int numContainTAMUOUt = 0;
	int numLinks = 0;
	int http2xx = 0;
	int http3xx = 0;
	int http4xx = 0;
	int http5xx = 0;
	int httpOther = 0;
};

char* request(const char* requestType, const char* request, const char* host, int* reqLen) {
	if (*reqLen > MAX_REQUEST_LEN) {
		return nullptr;
	}
	const char* format = "%s %s HTTP/1.0\r\nUser-agent: myTAMUcrawler/1.0\r\nHost: %s\r\nConnection: Close\r\n\r\n";
	*reqLen = strlen(format) + strlen(requestType) - 2 + strlen(request) - 2 + strlen(host) - 2;
	char* buf = new char[*reqLen + 1];
	sprintf(buf, format, requestType, request, host);
	return buf;
}

int processRobotHeader(SocketThreads& socketThreads) {
	char* header = socketThreads.buf;
	char* page = strstr(header, "\r\n\r\n");
	if (page == NULL) {
		return -1;
	}

	page[0] = '\0';
	page += 4;
	char* statusPtr = header + 9;
	int statusCode = atoi(statusPtr);
	return statusCode;
}

//-1 send fail, -2 read fail, -3 not return header
int requestURLThreads(SocketThreads& socketThreads, const char* request, int reqLen, const char* url, LPVOID param) {
	Parameters* p = (Parameters*)param;
	if (socketThreads.SendRequest(request, reqLen)) {
		clock_t t = clock();
		if (socketThreads.Read(false)) {
			char* header = socketThreads.buf;
			WaitForSingleObject(p->downloadMutex, INFINITE);
			p->numBytes += strlen(header);
			ReleaseMutex(p->downloadMutex);

			char* page = strstr(header, "\r\n\r\n");
			if (page == NULL) {
				return -3;
			}
			t = clock() - t;

			page[0] = '\0';
			page += 4;
			char* statusPtr = header + 9;
			int statusCode = atoi(statusPtr);
			//printf("receive from server,%d\n", statusCode);
			if (statusCode >= 200 && statusCode < 300) {
				WaitForSingleObject(p->httpMutex, INFINITE);
				p->http2xx++;
				ReleaseMutex(p->httpMutex);
				t = clock();
				HTMLParserBase HTMLPaser;
				char* baseURL = new char[strlen(url) + 1];
				strcpy(baseURL, url);
				int nLinks;
				char* parsedLinks = HTMLPaser.Parse(page, strlen(page), baseURL, strlen(baseURL), &nLinks);
				if (strstr(parsedLinks, "tamu.edu")!=NULL) {
					WaitForSingleObject(p->tamuMutex,INFINITE);
					p->numContainTAMU++;
					if (strstr(url, "tamu.edu") == NULL) {
						p->numContainTAMUOUt++;
					}
					ReleaseMutex(p->tamuMutex);
				}
				//printf("Parse a page\n");
				delete[] baseURL;
				t = clock() - t;
				return nLinks;// is negative, if error
			}
			else if (statusCode >= 300 && statusCode < 400) {
				WaitForSingleObject(p->httpMutex, INFINITE);
				p->http3xx++;
				ReleaseMutex(p->httpMutex);
			}
			else if (statusCode >= 400 && statusCode < 500) {
				WaitForSingleObject(p->httpMutex, INFINITE);
				p->http4xx++;
				ReleaseMutex(p->httpMutex);
			}
			else if (statusCode >= 500 && statusCode < 600) {
				WaitForSingleObject(p->httpMutex, INFINITE);
				p->http5xx++;
				ReleaseMutex(p->httpMutex);
			}
			WaitForSingleObject(p->httpMutex, INFINITE);
			p->httpOther++;
			ReleaseMutex(p->httpMutex);

			return 0;
		}
		return -2;
	}

	return -1;
}



void crawURL(SocketThreads& socketThreads, URLPaser& urlPaser, LPVOID param) {
	Parameters* p = (Parameters*)param;
	clock_t time;
	int prevSize;
	int reqLenRobot;
	char* requestRobot;

	if (!parseURL(urlPaser)) {
		return;
	}
	//check host uniqueness
	WaitForSingleObject(p->hostSetMutex, INFINITE);
	prevSize = p->seenHosts.size();
	p->seenHosts.insert(urlPaser.host);

	//unique
	if (prevSize < p->seenHosts.size()) {
		//printf("pass host unique\n");
		p->numUniqueHost += 1;
		//printf("Number host unique %d\n",p->numUniqueHost);
	}
	else
	{
		ReleaseMutex(p->hostSetMutex);
		return;
	}
	ReleaseMutex(p->hostSetMutex);


	if (!socketThreads.init()) {
		printf("socket initiate fail");
		return;
	}

	if (!socketThreads.DNS(urlPaser.host, urlPaser.port)) {
		
		//printf("OK DNS fail, the host is: %s \n url is: %s \n ", urlPaser.host, urlPaser.url);

		return;
	}
	//printf("pass DNS\n");


	DWORD IP = socketThreads.server.sin_addr.S_un.S_addr;
	WaitForSingleObject(p->IPSetMutex, INFINITE);
	p->numDNSPass++;
	prevSize = p->seenIPs.size();
	p->seenIPs.insert(IP);

	if (prevSize < p->seenIPs.size()) {
		//printf("pass IP\n");
		p->numUniqueIP += 1;
		//printf("Number IP unique%d\n",p->numUniqueIP);
	}
	else
	{
		ReleaseMutex(p->IPSetMutex);
		return;
	}
	ReleaseMutex(p->IPSetMutex);

	if (!socketThreads.Connect()) {
		//printf("tell me if connect fail\n");
		return;
	}
	//printf("pass Connect Robt\n");

	if ((requestRobot = request("HEAD", "/robots.txt", urlPaser.host, &reqLenRobot)) == NULL) {
		return;
	} // request need ot be deleted latter
	requestRobot[reqLenRobot] = '\0';
	//printf("Robt request:\n%s",requestRobot);

	if (!socketThreads.SendRequest(requestRobot, reqLenRobot)) {
		//printf("fail send the robot request\n");
		delete[] requestRobot;
		return;
	}
	//printf("successfully send the robot request\n");
	delete[] requestRobot;
	if (!socketThreads.Read(true)) {
		return;
	}

	int nRobBytes = strlen(socketThreads.buf);
	WaitForSingleObject(p->downloadMutex, INFINITE);
	p->numBytes += nRobBytes;
	ReleaseMutex(p->downloadMutex);
	int statusCode = 0;
	if ((statusCode = processRobotHeader(socketThreads)) < 0) {
		return;
	}
	

	

	if (statusCode >= 400) {
		WaitForSingleObject(p->robotMutex, INFINITE);
		p->numRobotPass++;
		ReleaseMutex(p->robotMutex);
		if (!socketThreads.reset()) {
			return;
		}
		int reqLen;
		char* requestPage;

		if ((requestPage = request("GET", urlPaser.request, urlPaser.host, &reqLen)) == NULL) {
			return;
		}// request need ot be deleted latter

		if (!socketThreads.Connect()) {
			delete[] requestPage;
			return;
		}
		int nlinksTempl = 0;
		
		// -1 means send fail, -2 means read fail, -3 means not return header, other nagetive may mean HTMLPaser error
		if ((nlinksTempl = requestURLThreads(socketThreads, requestPage, reqLen, urlPaser.url,param)) <0) {
			delete[] requestPage;
			return;
		}
		
		

		WaitForSingleObject(p->crawlMute, INFINITE);
		p->numCrawledPass++;
		p->numLinks+= nlinksTempl;
		ReleaseMutex(p->crawlMute);
		//printf("send final request\n%s\n",requestPage);
		delete[] requestPage;
	}


}



// return how many urls
int parseUrlsT(Parameters& pParams, char* fileName) {
	
	string curLine;
	ifstream file(fileName);
	if (!file.is_open()) {
		printf("open file fail\n");
		return -1;
	}
	
	while (getline(file, curLine))
	{
		pParams.urlsQue.push(curLine);
		//printf("%s\n",curLine.c_str());
	}
	return pParams.urlsQue.size();
	
}

DWORD crawlT(LPVOID param) {
	Parameters* p = (Parameters*)param;
	string url;
	while (true)
	{
		WaitForSingleObject(p->urlsQueMutex, INFINITE);
		if (p->urlsQue.size() <= 0) {
			break;
		}
		url = p->urlsQue.front();
		//printf("que size %d\n", p->urlsQue.size());
		p->numExtractedURLs++;
		p->urlsQue.pop();
		ReleaseMutex(p->urlsQueMutex);
		SocketThreads socketT;
		URLPaser urlPaser(url.c_str());
		crawURL(socketT, urlPaser, param);
		urlPaser.releaseMeme();
		socketT.close();
	}
	ReleaseMutex(p->urlsQueMutex);

	WaitForSingleObject(p->thredsQMutex, INFINITE);
	p->numThreadsQ--;
	ReleaseMutex(p->thredsQMutex);
	return 0;
}

DWORD statusT(LPVOID params) {
	Parameters* p = (Parameters*)params;
	bool lastPrint = false;
	clock_t startTime = clock();
	while (true) {
		int pages = p->numCrawledPass;
		double Mb = ((double)p->numBytes * 8)/1000000;
		Sleep(2000);
		Mb = ((double)p->numBytes * 8) /1000000 - Mb;
		pages = p->numCrawledPass - pages;
		double Mbps = Mb / 2;
		double pps = (double)pages / 2;
		printf("[%3d] %4d Q %6d E %7d H %6d D %6d I %5d R %5d C %5d L %4dK\n",
			(clock() - startTime) / CLOCKS_PER_SEC,
			p->numThreadsQ, (int)p->urlsQue.size(), p->numExtractedURLs, p->numUniqueHost, p->numDNSPass, p->numUniqueIP, p->numRobotPass,p->numCrawledPass,p->numLinks/1000);
		
		printf("\t*** crawling %.1f pps @ %.1f Mbps\n", pps,  Mbps );
		

		if (p->numThreadsQ == 0) {
			if (!lastPrint) {
				lastPrint = true;
			}
			else {
				break;
			}
			
		}
	}
	return 0;
}


int main(int argc, char* argv[])
{
	if (argc == 2) {
		char* url = argv[1];
		printf("URL: %s\n", url);
		URLPaser urlPaser(url);

		printf("\tParsing URL... ");
		
		if (!parseURL(urlPaser)) {
			return 0;
		}

		printf("host %s, port %d, request %s\n", urlPaser.host, urlPaser.port, urlPaser.request);
		Socket socket;
		if (!socket.DNS(urlPaser.host,urlPaser.port,true)) {
			return 0;
		}
		if (!socket.Connect()) {
			return 0;
		}
		requestURL(socket,urlPaser);
	}

	else if (argc == 3) {
		int threadsAmount = atoi(argv[1]);
		if (threadsAmount <= 0) {
			printf("Error,The amount of threads must larger than 0");
			return 0;
		}


		Parameters params;
		int totalUrls = parseUrlsT(params, argv[2]);
		//printf("%d,%d\n", params.urlsQue.size(),totalUrls);
		if (totalUrls < 0) {
			return 0;
		}

		clock_t startTimeClock = clock();

		WSADATA wsaData;
		WORD wVersionRequested = MAKEWORD(2, 2);
		if (WSAStartup(wVersionRequested, &wsaData) != 0) {
			printf("WSAStartup error %d\n", WSAGetLastError());
			WSACleanup();
			return 0;
		}

		params.urlsQueMutex = CreateMutex(NULL, FALSE, NULL);
		params.hostSetMutex = CreateMutex(NULL, FALSE, NULL);
		params.IPSetMutex = CreateMutex(NULL, FALSE, NULL);
		params.thredsQMutex = CreateMutex(NULL, FALSE, NULL);
		params.robotMutex = CreateMutex(NULL, FALSE, NULL);
		params.crawlMute = CreateMutex(NULL, FALSE, NULL);
		params.downloadMutex = CreateMutex(NULL, FALSE, NULL);
		params.tamuMutex = CreateMutex(NULL, FALSE, NULL);
		
		params.numThreadsQ = threadsAmount;

		HANDLE* crawlers = new HANDLE[threadsAmount];
		HANDLE stats = CreateThread(NULL, 0, (LPTHREAD_START_ROUTINE)statusT, &params, 0, NULL);
		for (int i = 0; i < threadsAmount; i++) {
			crawlers[i] = CreateThread(NULL, 0, (LPTHREAD_START_ROUTINE)crawlT, &params, 0, NULL);
			if (crawlers[i] == NULL) {
				printf("create threads fail");
				ExitProcess(3);
			}
		}
		

		for (int i = 0; i < threadsAmount; i++) {
			WaitForSingleObject(crawlers[i], INFINITE);
			CloseHandle(crawlers[i]);
		}

		double totalTime = (double)(clock() - startTimeClock) / CLOCKS_PER_SEC;

		WaitForSingleObject(stats, INFINITE);
		CloseHandle(stats);
		WSACleanup();
		printf("\nExtracted %d URLs @ %d/s\n", params.numExtractedURLs, (int)(params.numExtractedURLs / totalTime));
		printf("Looked up %d DNS names @ %d/s\n", params.numUniqueHost, (int)(params.numUniqueHost / totalTime));
		printf("Attempted %d robots @ %d/s\n", params.numUniqueIP, (int)(params.numUniqueIP / totalTime));
		printf("Crawled %d pages @ %d/s (%.2f MB)\n", params.numCrawledPass, (int)(params.numCrawledPass / totalTime), ((double)params.numBytes/1000000));
		printf("Parsed %d links @ %d/s\n", params.numLinks, (int)(params.numLinks / totalTime));
		printf("HTTP codes: 2xx = %d, 3xx = %d, 4xx = %d, 5xx = %d, other = %d\n", params.http2xx,params.http3xx,params.http4xx,params.http5xx,params.httpOther);
		printf("Number of pages contain a hyper link to tamu.edu: %d, ",params.numContainTAMU);
		printf("%d of them are outside\n", params.numContainTAMUOUt);

		ofstream myfile;
		myfile.open("UniqueIPs.txt");
		unordered_set<DWORD>::iterator itr;
		for (itr = params.seenIPs.begin(); itr != params.seenIPs.end();itr++) {
			char str[16];
			struct sockaddr_in sa;
			sa.sin_addr.S_un.S_addr = *itr;
			inet_ntop(AF_INET, &(sa.sin_addr), str, 16);
			myfile << str << '\n';
		}
		myfile.close();

		/*
		WSADATA wsaData;
		WORD wVersionRequested = MAKEWORD(2, 2);
		if (WSAStartup(wVersionRequested, &wsaData) != 0) {
			printf("WSAStartup error %d\n", WSAGetLastError());
			WSACleanup();
			return 0;
		}

		while (getline(file, curLine))
		{
			urls.push(curLine);
		}
		while (urls.size() > 0) {
			SocketThreads socketThreads;
			string url = urls.front();
			urls.pop();
			printf("URL: %s\n", url.c_str());
			URLPaser urlPaser(url.c_str());

			crawURL(socketThreads, urlPaser, seenHosts, seenIPs);
			urlPaser.releaseMeme();
			socketThreads.close();
		}
		WSACleanup();
		*/
	}

	return 0;
}



// Run program: Ctrl + F5 or Debug > Start Without Debugging menu
// Debug program: F5 or Debug > Start Debugging menu

// Tips for Getting Started: 
//   1. Use the Solution Explorer window to add/manage files
//   2. Use the Team Explorer window to connect to source control
//   3. Use the Output window to see build output and other messages
//   4. Use the Error List window to view errors
//   5. Go to Project > Add New Item to create new code files, or Project > Add Existing Item to add existing code files to the project
//   6. In the future, to open this project again, go to File > Open > Project and select the .sln file
