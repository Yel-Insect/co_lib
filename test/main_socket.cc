#include <iostream>
#include "co_lib.h"
#include "address.h"
#include "socket.h"
#include "iomanager.h"

static co_lib::Logger::ptr g_logger = LOG_ROOT();

void test_socket() {
	co_lib::IPAddress::ptr addr = co_lib::Address::LookupAnyIPAddress("www.baidu.com");
	if (addr) {
		LOG_INFO(g_logger) << "get address: " << addr->toString();
	} else {
		LOG_ERROR(g_logger) << "get address fail";
		return ;
	}

	co_lib::Socket::ptr sock = co_lib::Socket::CreateTCP(addr);
	addr->setPort(80);
	if (!sock->connect(addr)) {
		LOG_ERROR(g_logger) << "connect" << addr->toString() << " fail";
		return ;
	} else {
		LOG_INFO(g_logger) << "connect " << addr->toString() << " coonnected";
	}

	const char buff[] = "GET / HTTP/1.0\r\n\r\n";
	int rt = sock->send(buff, sizeof(buff));
	if (rt <= 0) {
		LOG_INFO(g_logger) << "send fail rt = " << rt;
		return ;
	}

	std::string buffs;
	buffs.resize(4096);
	rt = sock->recv(&buffs[0], buffs.size());

	if (rt <= 0) {
		LOG_INFO(g_logger) << "recv fail rt = " << rt;
		return ;
	}

	buffs.resize(rt);
	LOG_INFO(g_logger) << buffs;
}

int main(int argc, char** argv) {
	co_lib::IOManager iom;
	iom.schedule(&test_socket);
	return 0;
 }
