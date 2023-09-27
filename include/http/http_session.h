#ifndef __HTTP_SESSION_H__
#define __HTTP_SESSION_H__

//#include "socket_stream.h"
#include "socket.h"
#include "http.h"

namespace co_lib {

namespace http {

class HttpSession {//: public SocketStream {
public:
    typedef std::shared_ptr<HttpSession> ptr;
    HttpSession(Socket::ptr sock, bool owner = true);
    HttpRequest::ptr recvRequest();
    int sendResponse(HttpResponse::ptr rsp);
    int read(void* buffer, size_t length);
    int readFixSize(void* buffer, size_t length);
    int writeFixSize(const void* buffer, size_t length);
    int write(const void* buffer, size_t length);

    void close() { m_socket->close(); }
private:
    Socket::ptr m_socket;
};


}

}

#endif