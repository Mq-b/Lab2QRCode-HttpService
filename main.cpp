
#include <iostream>
#include <cassert>
#include <vector>
#include <random>
#include <algorithm>
#include <cstring>
#include <format>

#include <hv/HttpServer.h>

#include "src/compress.h"

constexpr int port = 10000;

int main() {
	hv::HttpService service;

	// 定义一个简单的 GET 接口
	service.GET("/ping", [](HttpRequest* req, HttpResponse* resp) {
		return resp->String("pong");
	});

	hv::HttpServer server;
	server.registerHttpService(&service);
	server.setPort(port);

	std::cout << std::format("Host Server on {}", port) << std::endl;
	server.run();

	return 0;
}
