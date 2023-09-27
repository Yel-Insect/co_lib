#include "http_server.h"
#include "log.h"

namespace co_lib {

static co_lib::Logger::ptr g_logger = LOG_NAME("system");

namespace http {

HttpServer::HttpServer(bool keepalive, co_lib::IOManager* worker,
                     co_lib::IOManager* accept_worker) 
    : TcpServer(worker, accept_worker)
    , m_isKeepalive(keepalive) {
    m_dispatch.reset(new ServletDispatch);
}

void HttpServer::handleClient(Socket::ptr client) {
    HttpSession::ptr session(new HttpSession(client));
    do {
        auto req = session->recvRequest();
        if (!req) {
            LOG_DEBUG(g_logger) << "recv http request fail, errno = "
                << errno << " errstr = " << strerror(errno)
                << " client: " << *client << " keep_alive = " << m_isKeepalive;
            break;
        }
        // LOG_DEBUG(g_logger) << *client << " keep_alive = " << m_isKeepalive;
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