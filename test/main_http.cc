#include <iostream>
#include "co_lib.h"
#include "http.h"



static co_lib::Logger::ptr g_logger = LOG_ROOT();
void test_request() {
	co_lib::http::HttpRequest::ptr req(new co_lib::http::HttpRequest);
	req->setHeader("host", "www.sylar.top");
	req->setBody("hello co_lib");

	req->dump(std::cout) << std::endl;
}

void test_response() {
	co_lib::http::HttpResponse::ptr rsp(new co_lib::http::HttpResponse);
	rsp->setHeader("X-X", "co_lib");
	rsp->setBody("hello co_lib");
	rsp->setClose(false);
	rsp->setStatus((co_lib::http::HttpStatus)400);

	rsp->dump(std::cout) << std::endl;
}


int main(int argc, char** argv) {
	test_request();
	test_response();
	return 0;
 }
