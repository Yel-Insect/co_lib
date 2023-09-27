#include <iostream>
#include <unistd.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include "co_lib.h"
#include "iomanager.h"
#include "hook.h"
#include "log.h"

static co_lib::Logger::ptr g_logger = LOG_ROOT();

void test_sleep() {
	co_lib::IOManager iom(1);
	iom.schedule([](){
		sleep(2);
		LOG_INFO(g_logger) << "sleep 2";
	});

	iom.schedule([](){
		sleep(3);
		LOG_INFO(g_logger) << "sleep 3";
	});
	LOG_INFO(g_logger) << "test_sleep";
}

void test_sock() {

	int sock = socket(AF_INET, SOCK_STREAM, 0);
	// fcntl(sock, F_SETFL, O_NONBLOCK);

	sockaddr_in addr;
	memset(&addr, 0, sizeof(addr));
	addr.sin_family = AF_INET;
	addr.sin_port = htons(80);
	inet_pton(AF_INET, "157.148.69.80", &addr.sin_addr.s_addr);

	LOG_INFO(g_logger) << "begin connect";
	int rt = connect(sock, (sockaddr*)&addr, sizeof(addr));
	LOG_INFO(g_logger) << "connect rt = " << rt << " errno = " << strerror(errno);
	if (rt) {
		return ;
	}

	const char data[] = "GET / HTTP/1.0\r\n\r\n";
	rt = send(sock, data, sizeof(data), 0);
	LOG_INFO(g_logger) << "send rt = " << rt << " errno = " << strerror(errno);
	if (rt <= 0) {
		return ;
	}

	std::string buff;
	buff.resize(4096);

	rt = recv(sock, &buff[0], buff.size(), 0);
	LOG_INFO(g_logger) << "recv rt = " << "errno = " << strerror(errno);

	if (rt <= 0) {
		return ;
	}

	buff.resize(rt);
	LOG_INFO(g_logger) << buff;
}


int main(int argc, char** argv) {
	//test_sleep();
	sleep_f(2);
	co_lib::IOManager iom;
	iom.schedule(test_sock);
	//test_sock();
	return 0;
 }
