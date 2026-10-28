#include<thread>
#include<vector>
#include<string>
#include"HtmlServer.h"
#include<WinSock2.h>
#include<memory>
#ifdef SetPort
#undef SetPort
#endif
#pragma comment(lib,"ws2_32.lib")
class WebServer
{
public:
	WebServer();
	WebServer* SetIP(const std::string&address);
	WebServer* SetPort(int port);
	WebServer* SetTimeout(float time);
	WebServer* SetSocket(int af, int type);
	WebServer* SetHostNumber(int length);
	WebServer* SetBufferSize(int);
	std::string GetAddress();
	int Start();
	int Work();
	void Stop();
private:
	void Request(SOCKET);
	void Send(SOCKET, const std::vector<char>&);
	int GetNetwoke();
	int WebMain();
	std::thread* webMainThread = nullptr;
	int hostNumber = 0;
	//基本信息区域
	std::string address = "";
	int port = 0;
	float time = 0.0f;
	int af = 0;
	int type = 0;

	int bufferSize = 1024;

	bool run = true;
	SOCKET serverSocket;
	SOCKADDR_IN addressIn;

	std::shared_ptr<HTMLServer> htmlServer;
	std::vector<std::shared_ptr<std::thread>> threadPool;
};

