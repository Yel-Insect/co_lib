#include <iostream>
#include "co_lib.h"
#include "iomanager.h"
#include "tcp_server.h"



static co_lib::Logger::ptr g_logger = LOG_ROOT();

void run() {
	auto addr = co_lib::Address::LookupAny("0.0.0.0:8033");
	//auto addr2 = co_lib::UnixAddress::ptr(new co_lib::UnixAddress("/tmp/unix_addr1"));
	std::vector<co_lib::Address::ptr> addrs;
	std::vector<co_lib::Address::ptr> fails;
	addrs.push_back(addr);
	//addrs.push_back(addr2);

	co_lib::TcpServer::ptr tcp_server(new co_lib::TcpServer);
	while (!tcp_server->bind(addrs, fails)) {
		sleep(2);
	}
	tcp_server->start();
}

int main(int argc, char** argv) {
	co_lib::IOManager iom(2);
	iom.schedule(run);
	return 0;
 }
