#include <iostream>
#include "co_lib.h"
#include "address.h"
#include "log.h"

static co_lib::Logger::ptr g_logger = LOG_ROOT();

void test() {
	std::vector<co_lib::Address::ptr> addrs;
	LOG_INFO(g_logger) << "start";
	bool v = co_lib::Address::Lookup(addrs, "www.baidu.com");
	LOG_INFO(g_logger) << "end";
	if (!v) {
		LOG_ERROR(g_logger) << "lookup fail";
		return ;
	}

	for (size_t i = 0; i < addrs.size(); i++) {
		LOG_INFO(g_logger) << i << " - " << addrs[i]->toString();
	}
}

// void test_iface() {
// 	// std::multimap<std::string, std::pair<sylar::Address::ptr, uint32_t>> results;
// 	std::vector<std::pair<co_lib::Address::ptr, uint32_t>> results;
// 	bool v = co_lib::Address::GetInterfaceAddresses(results, "lo");
// 	//LOG_INFO(g_logger) << "end";
// 	if (!v) {
// 		LOG_ERROR(g_logger) << "GetInterfaceAddresses fail";
// 		return ;
// 	}
// 	for (auto& i : results) {
// 		LOG_INFO(g_logger) << i.first << " - " << i.first->toString() << " - "
// 			<< i.second;
// 	}
// }

void test_ipv4() {
	auto addr = co_lib::IPAddress::Create("www.sylar.top");
	// auto addr = sylar::IPAddress::Create("www.baidu.com");

	if (addr) {
		LOG_INFO(g_logger) << addr->toString();
	}
}

int main(int argc, char** argv) {
	// test();
	// test_iface();
	test_ipv4();
	return 0;
 }
