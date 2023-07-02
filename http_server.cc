#include "http_server.h"
#include "log.h"

namespace sylar {

static sylar::Logger::ptr g_logger = SYLAR_LOG_NAME("system");

namespace http {

HttpServer::HttpServer(bool keepalive, sylar::IOManager* worker,
                     sylar::IOManager* accept_worker) 
    : TcpServer(worker, accept_worker)
    , m_isKeepalive(keepalive) {
}

void HttpServer::handleClient(Socket::ptr client) {

}

}

}