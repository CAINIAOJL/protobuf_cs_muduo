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

/*暂且不实现*/
//bool isSelfConnect(int sockfd);


}
}