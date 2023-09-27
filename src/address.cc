#include <sstream>
#include <netdb.h>
#include <ifaddrs.h>
#include "address.h"
//#include "endian.h"
#include "log.h"


namespace co_lib {

static co_lib::Logger::ptr g_logger = LOG_NAME("system");

bool Address::Lookup(std::vector<Address::ptr>& result, const std::string& host
                , int family, int type, int protocol) {
    addrinfo hints, *results, *next;
    // 初始化
    hints.ai_flags = 0;
    hints.ai_family = family;
    hints.ai_socktype = type;
    hints.ai_protocol = protocol;
    hints.ai_addrlen = 0;
    hints.ai_canonname = NULL;
    hints.ai_addr = NULL;
    hints.ai_next = NULL;

    std::string node;
    const char* service = NULL;

    service = (const char*)memchr(host.c_str(), ':', host.size());
    if (service) {
        // 判断后后面是否还有':'
        if (!memchr(service + 1, ':', host.c_str() + host.size() - service - 1)) {
            node = host.substr(0, service - host.c_str());  // 取地址
            service++;  // 取端口
        }
    }
    

    if (node.empty()) {
        node = host;
    }
    // 获取host的hints类型的套接字(hostname(IPv4 || IPv6) service(服务器名或端口号))
    int error = getaddrinfo(node.c_str(), service, &hints, &results);
    if (error) {
        LOG_ERROR(g_logger) << "Address::Lookup getaddress(" << host << ", "
            << family << ", " << type << ") err = " << error << " errstr = "
            << strerror(errno);
        return false;
    }

    next = results;
    // 遍历取出并封装为Address
    while (next) {
        result.push_back(Create(next->ai_addr, (socklen_t)next->ai_addrlen));
        next = next->ai_next;
    }
    // 释放内存
    freeaddrinfo(results);
    return true;
}

// 取任意一个Address
Address::ptr Address::LookupAny(const std::string& host, int family, int type, int protocol) {
    std::vector<Address::ptr> result;
    if (Lookup(result, host, family, type, protocol)) {
        return result[0];
    }
    return nullptr;
}

// 取任意一个IPAddress
IPAddress::ptr Address::LookupAnyIPAddress(const std::string& host, int family, int type, int protocol) {
    std::vector<Address::ptr> result;
    if (Lookup(result, host, family, type, protocol)) {
        for (auto& i : result) {
            IPAddress::ptr v = std::dynamic_pointer_cast<IPAddress>(i);
            if (v) {
                return v;
            }
        }
    }
    return nullptr;
}  



int Address::getFamily() const {
    return getAddr()->sa_family;
}

// 将ip转为字符串
std::string Address::toString() const {
    std::stringstream ss;
    insert(ss);
    return ss.str();
}

Address::ptr Address::Create(const sockaddr* addr, socklen_t addrlen) {
    if (addr == nullptr) {
        return nullptr;
    }
    // 判断socket的族
    Address::ptr result;
    switch(addr->sa_family) {
        case AF_INET:
            result.reset(new IPv4Address(*(const sockaddr_in*)addr));
            break;
        default:
            break;
    }
    return result;
}

bool Address::operator<(const Address& rhs) const {
    socklen_t minlen = std::min((uint32_t)getAddrLen(), (uint32_t)rhs.getAddrLen());
    int result = memcmp(getAddr(), rhs.getAddr(), minlen);
    if (result < 0) {
        return true;
    } else if (result > 0) {
        return false;
    } else if (getAddrLen() < rhs.getAddrLen()) {
        return true;
    }
    return false;
}

bool Address::operator==(const Address& rhs) const {
    return getAddrLen() == rhs.getAddrLen() 
        && memcmp(getAddr(), rhs.getAddr(), getAddrLen()) == 0;
}

bool Address::operator!=(const Address& rhs) const {
    return !(*this == rhs);
}


IPAddress::ptr IPAddress::Create(const char* address, uint16_t port) {
    addrinfo hints, *results;
    memset(&hints, 0, sizeof(addrinfo));

    hints.ai_flags= AI_NUMERICHOST; // address必须是一个地址串
    hints.ai_family = AF_UNSPEC;
    // 获取地址信息
    int error = getaddrinfo(address, NULL, &hints, &results);
    if (error) {
        LOG_ERROR(g_logger) << "IPAddress::Create(" << address
            << ", " << port << ") error = " << error
            << " error = " << errno << " errstr = " << strerror(errno); 
        return nullptr;
    }
    try {   // 是否能安全转换
        IPAddress::ptr result = std::dynamic_pointer_cast<IPAddress>(
            Address::Create(results->ai_addr, (socklen_t)results->ai_addrlen));
        if (result) {
            result->setPort(port);
        }
        freeaddrinfo(results);
        return result;
    } catch (...) {
        freeaddrinfo(results);
        return nullptr;
    }
}



IPv4Address::ptr IPv4Address::Create(const char* address, uint16_t port) {
    IPv4Address::ptr rt(new IPv4Address);
    rt->m_addr.sin_port = htons(port);
    int result = inet_pton(AF_INET, address, &rt->m_addr.sin_addr);
    if (result <= 0) {
        LOG_ERROR(g_logger) << "IPv4Address::Create(" << address << ", "
            << port << ") rt = " << result << " error = " << errno 
            << " errstr = " << strerror(errno); 
        return nullptr;
    }
    return rt;
}

IPv4Address::IPv4Address(const sockaddr_in& address) {
    m_addr = address;
}

IPv4Address::IPv4Address(uint32_t address, uint16_t port) {
    memset(&m_addr, 0, sizeof(m_addr));
    m_addr.sin_family = AF_INET;
    m_addr.sin_port = htons(port);
    m_addr.sin_addr.s_addr = htons(address);
}

sockaddr* IPv4Address::getAddr() {
    return (sockaddr*)&m_addr;
}

// 获取地址
const sockaddr* IPv4Address::getAddr() const {
    return (sockaddr*)&m_addr;
}

// 获取地址长度
socklen_t IPv4Address::getAddrLen() const {
    return sizeof(m_addr);
}

// 将地址输出到流中
std::ostream& IPv4Address::insert(std::ostream& os) const {
    uint32_t addr = ntohs(m_addr.sin_addr.s_addr);
    os << ((addr >> 24) & 0xff) << "."
       << ((addr >> 16) & 0xff) << "."
       << ((addr >> 8) & 0xff) << "."
       << (addr & 0xff);
    os << ":" <<  ntohs(m_addr.sin_port);
    return os;
}


// 获取端口号
uint32_t IPv4Address::getPort() const {
    return m_addr.sin_port;
}

// 设置端口号
void IPv4Address::setPort(uint16_t v) {
    m_addr.sin_port = htons(v);
}


std::ostream& operator<<(std::ostream& os, const Address& addr) {
    return addr.insert(os);
}

}