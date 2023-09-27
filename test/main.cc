
#include <iostream>
#include "http_server.h"
#include "address.h"
#include "socket.h"

void test_srv() {
    co_lib::http::HttpServer::ptr server(new co_lib::http::HttpServer);
    auto addr = co_lib::Address::LookupAnyIPAddress("0.0.0.0:8030");
    //addr->setPort(8030);
    while (!server->bind(addr)) {
        sleep(2);
    };

    // auto svl = server->getServletDispatch();
    // svl->addServlet("/xx", [](co_lib::http::HttpRequest::ptr req
    //                         , co_lib::http::HttpResponse::ptr rsp){
    //     rsp->setBody(req->toString());
    //     rsp->setHeader("Content-Length", std::to_string(req->toString().size()));
    // });
    // svl->addGlobServlet("/*", [](co_lib::http::HttpRequest::ptr req
    //                         , co_lib::http::HttpResponse::ptr rsp){
    //     rsp->setBody("Glob:\r\n" + req->toString());
    //     rsp->setHeader("Content-Length", std::to_string(req->toString().size()));
    // });
    server->start();
    
}

int main() {
    co_lib::IOManager iom(1);
    iom.schedule(test_srv);
    return 0;
}




