
#include <iostream>
#include "co_lib.h"
#include "iomanager.h"
#include "http_connection.h"
#if 0

static co_lib::Logger::ptr g_logger = LOG_ROOT();

void run() {
	co_lib::Address::ptr addr = co_lib::Address::LookupAnyIPAddress("www.baidu.com:80");
	if (!addr) {
		LOG_INFO(g_logger) << "get addr error";
	}
	co_lib::Socket::ptr sock = co_lib::Socket::CreateTCP(addr);
	bool rt = sock->connect(addr);
	if (!rt) {
		LOG_INFO(g_logger) << "connect " << *addr << " failed"; 
	}

	co_lib::http::HttpConnection::ptr conn(new co_lib::http::HttpConnection(sock));
	co_lib::http::HttpRequest::ptr req(new co_lib::http::HttpRequest);
	req->setPath("/blog/");
	req->setHeader("host", "www.co_lib.top");
	LOG_INFO(g_logger) << "req: " << std::endl
		<< *req;
	
	conn->sendRequest(req);
	auto rsp = conn->recvResponse();

	if (!rsp) {
		LOG_INFO(g_logger) << "recv response error";
		return ;
	}
	LOG_INFO(g_logger) << "rsp: " << std::endl
		<< *rsp;
}

#endif

void run() {

}
int main(int argc, char** argv) {
	co_lib::IOManager iom(2);
	iom.schedule(run);
	return 0;
 }
