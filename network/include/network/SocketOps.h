#pragma once 
#include <arpa/inet.h>

namespace network {
namespace socketops {


int createNoblocking_stop(sa_family_t family);

int connect(int sockfd, const struct sockaddr* addr);

void bind_stop(int sockfd, const struct sockaddr* addr);

int accept(int sockfd, struct sockaddr_in6 *addr);

void listen_stop(int sockfd);

ssize_t read(int sockfd, void* buf, size_t count);

ssize_t write(int sockfd, const void* buf, ssize_t count);

ssize_t readv(int sockfd, const struct iovec* iov, int iovcnt);

void close(int sockfd);

void shutdownwrite(int sockfd);


void toIpPort(char* buf, ssize_t size, const struct sockaddr* addr);
void toIp(char* buf, ssize_t size, const struct sockaddr* addr);

//ipv4
void fromIpPort(const char* ip, uint16_t port, struct sockaddr_in* addr);
//ipv6
void fromIpPort(const char* ip, uint16_t port, struct sockaddr_in6* addr);

int getSocketError(int sockfd);

//类型转换： sockaddr_in ->sockaddr
//ipv4
const struct sockaddr* sockaddr_cast(const struct sockaddr_in* addr);
//ipv6
const struct sockaddr* sockaddr_cast(const struct sockaddr_in6* addr);

//中间需要
struct sockaddr* sockaddr_cast(struct sockaddr_in6* addr);

//类型转换： sockaddr -> sockaddr_in
//ipv4
const struct sockaddr_in* sockaddr_in_cast(const struct sockaddr* addr);
//ipv6
const struct sockaddr_in6* sockaddr_in6_cast(const struct sockaddr* addr);

struct sockaddr_in6 getLocalAddr(int sockfd);
struct sockaddr_in6 getPeerAddr(int sockfd);

//判断是否是自连接,为什么自己连接自己
bool isSelfConnect(int sockfd) {
    //获取本地地址的相关信息
    struct sockaddr_in6 local = getLocalAddr(sockfd);
    //获取连接的对端地址的相关信息
    struct sockaddr_in6 peer = getPeerAddr(sockfd);
    //问题，为什么ipv6可以映射ipv4？
    //ipv4下
    if(local.sin6_family == AF_INET) {
        const struct sockaddr_in* localaddr = sockaddr_cast(&local);
        const struct sockaddr_in* peeraddr = sockaddr_cast(&peer);
        if(localaddr->sin_addr.s_addr == peeraddr->sin_addr.s_addr &&
           localaddr->sin_port == peeraddr->sin_port) {
                return true;
           }else {
            return false;
           }
        //return localaddr->sin_addr.s_addr == peeraddr->sin_addr.s_addr && localaddr->sin_port == peeraddr->sin_port
    } else if(local.sin6_family == AF_INET6) {
        //为什么对于ipv6来说，他的IP地址为什么调用memcmp，而不是直接比较？
        if(local.sin6_port == peer.sin6_port && memcmp(&local.sin6_addr, peer.sin6_addr, sizeof(local.sin6_addr)) == 0) {
            return true;
        } else {
            return false;
        }
    }
    return false;
}

};
}