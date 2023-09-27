#include <iostream>
#include <sys/types.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <fcntl.h>
#include <unistd.h>
#include "co_lib.h"
#include "iomanager.h"

static co_lib::Logger::ptr g_logger = LOG_ROOT();

int sock = 0;

void test_fiber() {
	LOG_INFO(g_logger) << "test_fiber";

	sock = socket(AF_INET, SOCK_STREAM, 0);
	fcntl(sock, F_SETFL, O_NONBLOCK);

	sockaddr_in addr;
	memset(&addr, 0, sizeof(addr));
	addr.sin_family = AF_INET;
	addr.sin_port = htons(80);
	inet_pton(AF_INET, "157.148.69.80", &addr.sin_addr.s_addr);

	if (!connect(sock, (sockaddr*)&addr, sizeof(addr))) {
		LOG_INFO(g_logger) <<"33333333333333333";
	} else if (errno == EINPROGRESS) {
		LOG_INFO(g_logger) << "add event errno = " << errno << " " << strerror(errno);

		co_lib::IOManager::GetThis()->addEvent(sock, co_lib::IOManager::READ, [](){
			LOG_INFO(g_logger) << "read callback";
		});
		LOG_INFO(g_logger) <<"1111111111111111111";
		co_lib::IOManager::GetThis()->addEvent(sock, co_lib::IOManager::WRITE, [&](){
			LOG_INFO(g_logger) << "write callback";
			//close(sock);
			co_lib::IOManager::GetThis()->cancelEvent(sock, co_lib::IOManager::READ);
		});
		LOG_INFO(g_logger) <<"22222222222222222222";
	} else {
		LOG_INFO(g_logger) << "else " << errno << " " << strerror(errno);
	}
}

void test1() {
	co_lib::IOManager iom(2, false);
	iom.schedule(&test_fiber);
}
co_lib::Timer::ptr s_timer;
void test_timer() {
	co_lib::IOManager iom(2);
	s_timer = iom.addTimer(1000, [](){
		LOG_INFO(g_logger) << "hello timer";
		static int i = 0;
		if (++i == 3) {
			s_timer->reset(2000, true);
			//s_timer->cancel();
		}
 	}, true);
}

int main(int argc, char** argv) {
	//test1();
	test_timer();
	return 0;
 }
