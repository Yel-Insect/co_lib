#include <iostream>
#include "sylar.h"
#include "iomanager.h"
#include "http_connection.h"
#include "uri.h"


static sylar::Logger::ptr g_logger = SYLAR_LOG_ROOT();

void test_pool() {
	sylar::http::HttpConnectionPool::ptr pool(new sylar::http::HttpConnectionPool(
		"www.sylar.top", "", 80, 10, 1000 * 30, 5));
	sylar::IOManager::GetThis()->addTimer(1000, [pool](){
		auto r = pool->doGet("/", 300);
		SYLAR_LOG_INFO(g_logger) << r->toString();
	}, true);
}

// void run() {
// 	sylar::Address::ptr addr = sylar::Address::LookupAnyIPAddress("www.baidu.com:80");
// 	if (!addr) {
// 		SYLAR_LOG_INFO(g_logger) << "get addr error";
// 	}
// 	sylar::Socket::ptr sock = sylar::Socket::CreateTCP(addr);
// 	bool rt = sock->connect(addr);
// 	if (!rt) {
// 		SYLAR_LOG_INFO(g_logger) << "connect " << *addr << " failed"; 
// 	}

// 	sylar::http::HttpConnection::ptr conn(new sylar::http::HttpConnection(sock));
// 	sylar::http::HttpRequest::ptr req(new sylar::http::HttpRequest);
// 	req->setPath("/blog/");
// 	req->setHeader("host", "www.sylar.top");
// 	SYLAR_LOG_INFO(g_logger) << "req: " << std::endl
// 		<< *req;
	
// 	conn->sendRequest(req);
// 	auto rsp = conn->recvResponse();

// 	if (!rsp) {
// 		SYLAR_LOG_INFO(g_logger) << "recv response error";
// 		return ;
// 	}
// 	SYLAR_LOG_INFO(g_logger) << "rsp: " << std::endl
// 		<< *rsp;
// }

int main(int argc, char** argv) {
	// sylar::Uri::ptr uri = sylar::Uri::Create("http://admin@www.sylar.top/test/中文/uri?id=100&name=sylar&vv=中文#frg中文");
	// std::cout << uri->toString() << std::endl;
	// auto addr = uri->createAddress();
	// std::cout << *addr << std::endl;
	sylar::IOManager iom(2);
	iom.schedule(test_pool);

	// auto r = sylar::http::HttpConnection::DoGet("http://www.sylar.top/blog/", 300);
	// SYLAR_LOG_INFO(g_logger) << "result = " << r->result
	// 	<< " error = " << r->error
	// 	<< " rsp = " << (r->response ? r->response->toString() : "");
	
	return 0;
 }
