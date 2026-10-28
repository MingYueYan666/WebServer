#include <iostream>

#include"WebServer.h"

int main() {
	WebServer web = WebServer();
	web.SetIP("0.0.0.0");
	web.SetPort(8080);
	web.SetTimeout(800);
	web.SetSocket(AF_INET, SOCK_STREAM);
	web.SetHostNumber(10);
	web.SetBufferSize(1024 * 128);
	return web.Start();
} 

