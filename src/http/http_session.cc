#include "http_session.h"
#include "http_parser.h"
#include "log.h"

namespace co_lib {

static co_lib::Logger::ptr g_logger = LOG_NAME("system");

namespace http {

HttpSession::HttpSession(Socket::ptr sock, bool owner) 
    : m_socket(sock){
}

// 接收报文
HttpRequest::ptr HttpSession::recvRequest() {
    HttpRequestParser::ptr parser(new HttpRequestParser);
    uint64_t buff_size = HttpRequestParser::GetHttpRequestBufferSize();

    std::shared_ptr<char> buffer(new char[buff_size], [](char* ptr) {
        delete[] ptr;
    });
    char* data = buffer.get();
    int offset = 0;
    do {
        int len = read(data + offset, buff_size - offset);
        if (len <= 0) {
            m_socket->close();
            return nullptr;
        }
        len += offset;
        size_t nparse = parser->execute(data, len);
        if (parser->hasError()) {
            m_socket->close();
            return nullptr;
        }
        offset = len - nparse;
        if (offset == (int)buff_size) {
            m_socket->close();
            return nullptr;
        }
        if (parser->isFinished()) {
            break;
        }
    } while (true);
    int64_t length = parser->getContentLength();
    if (length > 0) {
        std::string body;
        body.resize(length);

        int len = 0;
        if (length >= offset) {
            memcpy(&body[0], data, offset);
            len = offset;
        } else {
            memcpy(&body[0], data,length);
            len = length;
        }
        length -= offset;
        if (length > 0) {
            if (readFixSize(&body[len], length) <= 0) {
                m_socket->close();
                return nullptr;
            }
        }
        parser->getData()->setBody(body);
    }
    std::string keep_alive = parser->getData()->getHeader("Connection");
    if (!strcasecmp(keep_alive.c_str(), "keep-alive")) {
        parser->getData()->setClose(false);
    }
    return parser->getData();
}



// 发送报文
int HttpSession::sendResponse(HttpResponse::ptr rsp) {
    std::stringstream ss;
    ss << *rsp;
    std::string data = ss.str();
    return writeFixSize(data.c_str(), data.size());
}



int HttpSession::read(void* buffer, size_t length) {
    if (!m_socket->isConnected()) {
        return -1;
    }
    return m_socket->recv(buffer, length);
}

int HttpSession::readFixSize(void* buffer, size_t length) {
    size_t offset = 0;
    size_t left = length;
    while (left > 0) {
        size_t len = read((char*)buffer + offset, left);
        if (len <= 0) {
            return len;
        }
        offset += len;
        left -= len;
    }
    return length;
}

int HttpSession::writeFixSize(const void* buffer, size_t length) {
    size_t offset = 0;
    size_t left = length;
    while (left > 0) {
        size_t len = write((const char*)buffer + offset, left);
        if (len <= 0) {
            return len;
        }
        offset += len;
        left -= len;
    }
    return length;
}

int HttpSession::write(const void* buffer, size_t length) {
    if (!m_socket->isConnected()) {
        return -1;
    }
    return m_socket->send(buffer, length);
}


}

}