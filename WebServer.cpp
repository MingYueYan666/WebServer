#include "WebServer.h"
#include<iostream>
#include<mutex>
WebServer::WebServer() : webMainThread(nullptr) {
	htmlServer = std::make_shared<HTMLServer>("All", "root");
	htmlServer->Set404("404.html");
	htmlServer->SetMain("index.html");
	htmlServer->SetDefaultIco("default.png");
	GetNetwoke();
}
int WebServer::WebMain() {
	return 0;
}
int WebServer::Start() {
	serverSocket = socket(af,type,0);
	if (serverSocket == INVALID_SOCKET) {
		return -1;
	}
	addressIn.sin_family = af;
	addressIn.sin_addr.s_addr = inet_addr(address.c_str());
	addressIn.sin_port = htons(port);

	if (bind(serverSocket, (SOCKADDR*)&addressIn, sizeof(addressIn)) == SOCKET_ERROR) {
		std::cout << "bind error" << std::endl;
		return -2;
	};
	if (listen(serverSocket, hostNumber) == SOCKET_ERROR) {
		std::cout << "listen error" << std::endl;
		return -3;
	}
	run = true;
	return Work();
}
void WebServer::Stop() {
	run = false;
}
int WebServer::Work() {
	while (run) {
	   SOCKET clientSocket = accept(serverSocket, NULL, NULL);
	   Request(clientSocket);
	  /* std::shared_ptr<std::thread> thread = std::make_shared<std::thread>(&WebServer::Request, this, clientSocket);
	   threadPool.push_back(thread);*/
	}
	return 0;
}
void WebServer::Request(SOCKET clientSocket) {
	char* buffer = new char[bufferSize];
	int make = 0;
	while ((make = recv(clientSocket, buffer, bufferSize, 0)) > 0) {
		if (make >= bufferSize) {
			std::cout << "buffer Overflow" << std::endl;
			closesocket(clientSocket);
			break;
		}
		buffer[make] = '\0';
		auto response = htmlServer->GetData(buffer);
		auto data = response.GetData();
		auto result = send(clientSocket, data.data(), data.size(), 0);
		if(response.IsBlock) { //是块
			int remainder = response.remainder;
			int pos = response.pos;
			while (remainder >0) {
				auto block = htmlServer->GetBlock(response.fileName, pos, bufferSize);
				int size = bufferSize> remainder ? bufferSize : remainder;
				int result = send(clientSocket, block.data(), size, 0);
				if (result == SOCKET_ERROR)break;
				remainder -= size;
				pos += size;
			}
		}
	}
	closesocket(clientSocket);
	std::cout << "close client socket" << std::endl;
	delete[] buffer;
}
void WebServer::Send(SOCKET clientSocket, const std::vector<char>& response) {
	size_t offset = 0;
	size_t remaining = response.size();

	while (remaining > 0) {
		// 不用 std::min 的手动计算
		int sendSize = (remaining > static_cast<size_t>(bufferSize))
			? bufferSize
			: static_cast<int>(remaining);

		int sent = send(clientSocket, response.data() + offset, sendSize, 0);
		if (sent == SOCKET_ERROR) {
			std::cout << "send error" << std::endl;
			break;
		}

		// 必须用实际发送的字节数更新 offset 和 remaining
		offset += sent;
		remaining -= sent;

		// 可选进度显示（注意浮点转换）
		float progress = static_cast<float>(offset) / response.size();
		std::cout << "progress " << progress << std::endl;
	}
	std::cout << "send complete" << std::endl;
}

WebServer* WebServer::SetIP(const std::string& address) {
	this->address = address;
	return this;
}
WebServer* WebServer::SetPort(int port) {
	this->port = port;
	return this;
}
WebServer* WebServer::SetTimeout(float time) {
	this->time = time;
	return this;
}
WebServer* WebServer::SetSocket(int af, int type) {
	this->af = af;
	this->type = type;
	return this;
}
WebServer* WebServer::SetHostNumber(int length) {
	hostNumber = length;
	return this;
}
WebServer* WebServer::SetBufferSize(int size) {
	bufferSize = size;
	return this;
}


std::string WebServer::GetAddress() {
	return address +":"+ std::to_string(port);
}
int WebServer::GetNetwoke() {
	WSADATA wsadata;
	return WSAStartup(MAKEWORD(2,2),&wsadata);
}

