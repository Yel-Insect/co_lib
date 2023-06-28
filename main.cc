#include <iostream>
#include "sylar.h"
#include "http.h"



static sylar::Logger::ptr g_logger = SYLAR_LOG_ROOT();
void test_request() {
	sylar::http::HttpRequest::ptr req(new sylar::http::HttpRequest);
	req->setHeader("host", "www.sylar.top");
	req->setBody("hello sylar");

	req->dump(std::cout) << std::endl;
}

void test_response() {
	sylar::http::HttpResponse::ptr rsp(new sylar::http::HttpResponse);
	rsp->setHeader("X-X", "sylar");
	rsp->setBody("hello sylar");
	rsp->setClose(false);
	rsp->setStatus((sylar::http::HttpStatus)400);

	rsp->dump(std::cout) << std::endl;
}


int main(int argc, char** argv) {
	test_request();
	test_response();
	return 0;
 }
