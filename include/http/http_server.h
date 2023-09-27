#ifndef __HTTP_SERVER_H__
#define __HTTP_SERVER_H__

#include "tcp_server.h"
#include "http_session.h"
#include "servlet.h"

namespace co_lib {

namespace http {

class HttpServer : public TcpServer {
public:
    typedef std::shared_ptr<HttpServer> ptr;
    HttpServer(bool keepalive = false,
               co_lib::IOManager* worker = co_lib::IOManager::GetThis(),
               co_lib::IOManager* accept_worker = co_lib::IOManager::GetThis());

    ServletDispatch::ptr getServletDispatch() const { return m_dispatch; }
    void setServletDispatch(ServletDispatch::ptr v) { m_dispatch = v; }
protected:
    virtual void handleClient(Socket::ptr client) override;
private:
    bool m_isKeepalive;
    ServletDispatch::ptr m_dispatch;
};


}


}

#endif