#include "http_server.h"
#include "log.h"

namespace sylar {

static sylar::Logger::ptr g_logger = SYLAR_LOG_NAME("system");

namespace http {

HttpServer::HttpServer(bool keepalive, sylar::IOManager* worker,
                     sylar::IOManager* accept_worker) 
    : TcpServer(worker, accept_worker)
    , m_isKeepalive(keepalive) {
    m_dispatch.reset(new ServletDispatch);
}

void HttpServer::handleClient(Socket::ptr client) {
    HttpSession::ptr session(new HttpSession(client));
    do {
        auto req = session->recvRequest();
        if (!req) {
            SYLAR_LOG_DEBUG(g_logger) << "recv http request fail, errno = "
                << errno << " errstr = " << strerror(errno)
                << " client: " << *client << " keep_alive = " << m_isKeepalive;
            break;
        }
        // SYLAR_LOG_DEBUG(g_logger) << *client << " keep_alive = " << m_isKeepalive;
        // HttpResponse::ptr rsp(new HttpResponse(req->getVersion(), req->isClose() || !m_isKeepalive));
        // m_dispatch->handle(req, rsp, session);

        // session->sendResponse(rsp);
        HttpResponse::ptr rsp(new HttpResponse(req->getVersion()
                            ,req->isClose() || !m_isKeepalive));
        rsp->setHeader("Server", getName());
        m_dispatch->handle(req, rsp, session);
        session->sendResponse(rsp);

        if(!m_isKeepalive || req->isClose()) {
            break;
        }
    } while (true);
    session->close();
}

}

}