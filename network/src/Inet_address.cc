//#include "network/include/Inet_address.h"
//#include "network/include/Endian.h"
#include "network/Endian.h"
#include "network/Inet_address.h"
#include <netdb.h>
//#include <netinet/in.h>
#include <cassert>
#include <sys/socket.h>
#include <string.h>

/*采用google日志库*/
#include <glog/logging.h>

//这段代码的主要功能是定义两个与IPv4地址相关的常量，用于网络编程中的地址处理，同时调整GCC编译器对旧式类型转换的警告和错误处理。这些常量的定义为网络应用提供了一种标准化的方法来表示任何地址和本地回环地址，方便在进行 socket 编程时使用
#pragma GCC diagnostic ignored "-Wold-style-cast"
static const in_addr_t KInaddrAny = INADDR_ANY;
/*INADDR_LOOPBACK = Inet 127.0.0.1.*/
static const in_addr_t KInaddrLoopbacl = INADDR_LOOPBACK;
#pragma GCC diagnostic error "-Wold-style-cast"

using namespace network;

//验证
static_assert(sizeof(Inet_address) == sizeof(struct sockaddr_in6),
    "Inet_address size not match with sockaddr_in6");

static_assert(offsetof(sockaddr_in, sin_family) == 0, "sockaddr sa_family offset error");
static_assert(offsetof(sockaddr_in6, sin6_family ) == 0, "sockaddr_in6 sa_family offset error");


/**
 * @return Inet_address类
 * @param port 端口号
 * @param Loopbackonly 是否只允许本地回环地址
 * @param ipv6 是否使用ipv6
 */
Inet_address::Inet_address(uint16_t port, bool Loopbackonly, bool ipv6) {
    if(ipv6 == true) {
        memset(&addr_6, 0, sizeof(addr_6));
        //设置
        addr_6.sin6_family = AF_INET6;
        //addr_6.sin6_addr = static_cast<in6_addr>(htons(Loopbackonly == true ? KInaddrLoopbacl : KInaddrAny));
        //ip
        in6_addr ip = Loopbackonly == true ? in6addr_loopback : in6addr_any;
        //ipv6 32位地址
        addr_6.sin6_addr = ip;
        //端口
        addr_6.sin6_port = endian::host_to_net_32(port);
    } else {
        //ipv4
        memset(&addr_4, 0, sizeof(addr_4));
        addr_4.sin_family = AF_INET;
        in_addr_t ip = Loopbackonly == true ? KInaddrLoopbacl : KInaddrAny;
        addr_4.sin_addr.s_addr = endian::host_to_net_32(ip);
        //端口
        addr_4.sin_port = endian::host_to_net_16(port);  
    }
}

/**
 * @return Inet_address类
 * @param ip 地址
 * @param port 端口号
 * @param Loopbackonly 是否只允许本地回环地址
 * @param ipv6 是否使用ipv6
 */
Inet_address::Inet_address(const std::string ip, uint16_t port, bool Loopbackonly, bool ipv6) {
    //ip.c_str()返回一个指向以null结尾的字符串的指针，C语言风格
    if(ipv6 || strchr(ip.c_str(), ':')) {
        //ipv6
        memset(&addr_6, 0, sizeof(addr_6));
        addr_6.sin6_family = AF_INET6;
        //ip
        socketops::fromIpPort(ip.c_str(), port, &addr_6);
    } else {
        memset(&addr_4, 0, sizeof(addr_4));
        addr_4.sin_family = AF_INET;
        socketops::fromIpPort(ip.c_str(), port, &addr_4);
    }
}

std::string Inet_address::Ip_to_string() const {
    char buf[64];
    memset(buf, 0, sizeof(buf));
    socketops::toIp(buf, sizeof(buf), getSockAddr());
    return buf;
}

std::string Inet_address::Ip_Port_to_string() const {
    char buf[64];
    memset(buf, 0, sizeof(buf));
    socketops::toIpPort(buf, sizeof(buf), getSockAddr());
    return buf;
}

/**
 * @return 返回ipv4的端口（主机序）
 */
uint16_t Inet_address::port() const {
    return endian::net_to_host_16(portNetEndian());
}

/**
 * @return 返回ipvd的IP地址（网络字节序）
 */
uint32_t Inet_address::ipv4NetEndian() const {
    assert(family_4() == AF_INET);
    return addr_4.sin_addr.s_addr;
}


/*每个线程 buffer 独一份 */
/*__thread 控制了每个线程的栈空间，每个线程都有自己独立的栈空间，互不干扰，因此可以实现线程安全的全局变量。*/
static __thread char buffer[64 * 1024];

/**
 * @return 成功为true，失败为false
 * @brief 解析主机名，并将结果存入result中，详细看unix(套接字联网API) 卷一 11章 
 * @param hostname 主机名
 * @param result 存放结果
 */
bool Inet_address::reslove(const std::string &hostname, Inet_address * result) {
    assert(result != NULL);
    struct hostent hent;
    struct hostent * he = NULL;/*用于存放结果*/
    int herro = 0;
    memset(&hent, 0, sizeof(hent));
    //gethostbyname_r是线程安全的，但是可能存在竞争条件，所以不推荐使用
    //herro = gethostbyname_r(hostname.c_str(), &hent, buffer, sizeof(buffer), &he, &h_errnop);
    //使用gethostbyname_r_np代替gethostbyname_r，它是线程安全的
    int ret = gethostbyname_r(hostname.c_str(), &hent, buffer, sizeof(buffer), &he, &herro);

    if(ret == 0 && he != NULL) {
        //successed
        assert(he->h_addrtype == AF_INET && he->h_length == sizeof(uint32_t));
        /*#define h_addr h_addr_list[0]*/
        result->addr_4.sin_addr = *reinterpret_cast<struct in_addr*>(he->h_addr);
        //result->addr_4.sin_addr = *reinterpret_cast<struct in_addr*>(he->h_addr_list[0]);
        return true;
    } else {
        if(ret) {
            LOG(ERROR) << "Inet_address::resolve error!";
        }
        return false;
    }
}