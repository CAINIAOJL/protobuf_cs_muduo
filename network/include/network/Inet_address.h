#pragma once
#include <netinet/in.h>
#include "SocketOps.h"
#include <string>
namespace network {
//网络地址类
class Inet_address {
public:
 //默认构造函数，构造一个IPv4地址，默认端口为12345
 /*INADDR_LOOPBACK = Inet 127.0.0.1.*/
 /*Loopbackonly = 127.0.0.1 */
 explicit Inet_address(uint16_t port = 12345, bool Loopbackonly = false, bool ipv6 = false);

 //给出ip地址和端口号的构造函数
 Inet_address(const std::string ip, uint16_t port, bool Loopbackonly = false, bool ipv6 = false);

 /* Structure describing an Internet socket address.  */
 //ipv4
 explicit Inet_address(const sockaddr_in& addr) : addr_4(addr) {};

 //ipv6
 explicit Inet_address(const sockaddr_in6& addr) : addr_6(addr) {};

 sa_family_t family_4() const {return addr_4.sin_family;}
 sa_family_t family_6() const {return addr_6.sin6_family;}
 
 std::string Ip_to_string() const;
 std::string Ip_Port_to_string() const;

 uint16_t port() const;

 //其他功能 
 //ipv4网络字节序
 uint32_t ipv4NetEndian() const;
 uint16_t  portNetEndian() const {
    return addr_4.sin_port;
 }
 //ipv6
 void setSockAddrInet6 (const sockaddr_in6& addr) {addr_6 = addr;}

 /**
  * @return 指向sockaddr的指针
  */
 const sockaddr* getSockAddr() const {
  return socketops::sockaddr_cast(&addr_6);
 }

 //忽略 void setScopeId(uint32_t scope_id); 

 static bool reslove(const std::string &hostname, Inet_address * result);

 private:
 //管理ipv4和ipv6地址的union
  union {
    sockaddr_in addr_4;
    sockaddr_in6 addr_6;
  };
};

}