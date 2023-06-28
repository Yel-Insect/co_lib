#ifndef __HTTP_PARSER_H__
#define __HTTP_PARSER_H__

#include <memory>
#include "http.h"
#include "http/http11_parser.h"
#include "http/httpclient_parser.h"

namespace sylar {

namespace http {

class HttpRequestParser {
public:
    typedef std::shared_ptr<HttpRequestParser> ptr;
    HttpRequestParser();
    size_t execute(const char* data, size_t len, size_t off);
    int isFinished() const;
    int hasError() const;

    HttpRequest::ptr getData() const { return m_data; }
    void setError(int v) { m_error = v; }
private:
    http_parser m_parser;
    HttpRequest::ptr m_data;
    int m_error;
};

class HttpResponseParser {
public:
    typedef std::shared_ptr<HttpRequestParser> ptr;
    HttpResponseParser();
    size_t execute(const char* data, size_t len, size_t off);
    int isFinished() const;
    int hasError() const;

    HttpResponse::ptr getData() const { return m_data; }
private:
    httpclient_parser m_parser;
    HttpResponse::ptr m_data;
    int m_error;
};

}

}

#endif