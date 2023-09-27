#if 0
#include <iostream>
#include "co_lib.h"
#include "iomanager.h"
#include "tcp_server.h"
#include "bytearray.h"



static co_lib::Logger::ptr g_logger = LOG_ROOT();

class EchoServer : public co_lib::TcpServer {
public:
	EchoServer(int type);
	void handleClient(co_lib::Socket::ptr client);

private:
	int m_type = 0;
};

EchoServer::EchoServer(int type) 
	: m_type(type){
}

void EchoServer::handleClient(co_lib::Socket::ptr client) {
	LOG_INFO(g_logger) << "handleClient " << *client;
	co_lib::ByteArray::ptr ba(new co_lib::ByteArray);
	while (true) {
		ba->clear();
		std::vector<iovec> iovs;
		ba->getWriteBuffers(iovs, 1024);

		int rt = client->recv(&iovs[0], iovs.size());
		if (rt == 0) {
			LOG_INFO(g_logger) << "client close: " << *client;
			break;
		} else if (rt < 0) {
			LOG_INFO(g_logger) << "client error rt = : " << rt
				<< " errno = " << errno << " errstr = " << strerror(errno);
			break;
		}
		ba->setPosition(ba->getPosition() + rt);
		ba->setPosition(0);
		if (m_type == 1) {
			LOG_INFO(g_logger) << ba->toString();
		} else {
			LOG_INFO(g_logger) << ba->toHexString();
		}
	}
}

int type = 1;

void run() {
	EchoServer::ptr es(new EchoServer(type));
	auto addr = co_lib::Address::LookupAny("0.0.0.0:8020");
	while (!es->bind(addr)) {
		sleep(2);
	}
	es->start();
}

int main(int argc, char** argv) {
	if (argc < 2) {
		LOG_INFO(g_logger) << "used as[" << argv[0] << " -t] or [" << argv[0] << " - b]";
		return 0;
	}

	if (!strcmp(argv[1], "-b")) {
		type = 2;
	}
	co_lib::IOManager iom(2);
	iom.schedule(run);
	return 0;
 }
#endif

int main() {
	return 0;
}