#include <iostream>
#include "sylar.h"
#include "iomanager.h"
#include "http_connection.h"
#include "uri.h"


static sylar::Logger::ptr g_logger = SYLAR_LOG_ROOT();



int main(int argc, char** argv) {
	sylar::Uri::ptr uri = sylar::Uri::Create("http://admin@www.sylar.top/test/中文/uri?id=100&name=sylar&vv=中文#frg中文");
	std::cout << uri->toString() << std::endl;
	auto addr = uri->createAddress();
	std::cout << *addr << std::endl;
	return 0;
 }
