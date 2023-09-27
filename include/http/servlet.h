#ifndef __SERVLET_H__
#define __SERVLET_H__

#include <memory>
#include <functional>
#include <string>
#include <vector>
#include <unordered_map>
#include "http.h"
#include "http_session.h"
#include "thread.h"

namespace co_lib {

namespace http {

class Servlet {
public:
    typedef std::shared_ptr<Servlet> ptr;
    Servlet(const std::string& name) 
        :m_name(name) {}
    virtual ~Servlet() {}
    virtual int32_t handle(co_lib::http::HttpRequest::ptr request
                         , co_lib::http::HttpResponse::ptr response
                         , co_lib::http::HttpSession::ptr session) = 0;

protected:
    std::string m_name;
};

class FunctionServlet : public Servlet {
public:
    typedef std::shared_ptr<FunctionServlet> ptr;
    typedef std::function<int32_t (co_lib::http::HttpRequest::ptr request
                         , co_lib::http::HttpResponse::ptr response
                         , co_lib::http::HttpSession::ptr session)> callback;

    FunctionServlet(callback cb);
    virtual int32_t handle(co_lib::http::HttpRequest::ptr request
                         , co_lib::http::HttpResponse::ptr response
                         , co_lib::http::HttpSession::ptr session) override;
private:
    callback m_cb;
};

class ServletDispatch {
public:
    typedef std::shared_ptr<ServletDispatch> ptr;
    typedef RWMutex RWMutexType;

    ServletDispatch();
    int32_t handle(co_lib::http::HttpRequest::ptr request
                 , co_lib::http::HttpResponse::ptr response
                 , co_lib::http::HttpSession::ptr session);

    void addServlet(const std::string& uri, Servlet::ptr slt);
    void addServlet(const std::string& uri, FunctionServlet::callback cb);
    void addGloServlet(const std::string& uri, Servlet::ptr slt);
    void addGloServlet(const std::string& uri, FunctionServlet::callback cb);

    void delServlet(const std::string& uri);
    void delGloServlet(const std::string& uri);

    Servlet::ptr getDefault() const { return m_default; }
    void setDefault(Servlet::ptr v) { m_default = v; }

    Servlet::ptr getServlet(const std::string& uri);
    Servlet::ptr getGloServlet(const std::string& uri);

    Servlet::ptr getMatchedServlet(const std::string& uri);
private:
    RWMutexType m_mutex;
    // uri(/co_lib/xxx) -> servlet
    std::unordered_map<std::string, Servlet::ptr> m_datas;
    // uri(/co_lib/*) -> servlet
    std::vector<std::pair<std::string, Servlet::ptr>> m_globs;
    // 默认servlet， 所有的路径都没匹配到时使用
    Servlet::ptr m_default;

};

class NotFoundServlet : public Servlet {
public:
    typedef std::shared_ptr<NotFoundServlet> ptr;
    NotFoundServlet();
    virtual int32_t handle(co_lib::http::HttpRequest::ptr request
                         , co_lib::http::HttpResponse::ptr response
                         , co_lib::http::HttpSession::ptr session) override;
};
}


}

#endif