#include <iostream>
#include "co_lib.h"
#include "iomanager.h"
#include "http_server.h"
#include "servlet.h"



static co_lib::Logger::ptr g_logger = LOG_ROOT();

void run() {
	co_lib::http::HttpServer::ptr server(new co_lib::http::HttpServer);
	co_lib::Address::ptr addr = co_lib::Address::LookupAnyIPAddress("0.0.0.0:8020");
	while (!server->bind(addr)) {
		sleep(2);
	}
	auto sd = server->getServletDispatch();
	sd->addServlet("/xx", [](co_lib::http::HttpRequest::ptr req
				, co_lib::http::HttpResponse::ptr rsp
				, co_lib::http::HttpSession::ptr session) {
			rsp->setBody(req->toString());
			return 0;
		});
	sd->addGloServlet("/*", [](co_lib::http::HttpRequest::ptr req
				, co_lib::http::HttpResponse::ptr rsp
				, co_lib::http::HttpSession::ptr session) {
			rsp->setBody("Glob:\r\n" + req->toString());
			return 0;
		});
	server->start();
}

int main(int argc, char** argv) {
	co_lib::IOManager iom(2);
	iom.schedule(run);
 }
