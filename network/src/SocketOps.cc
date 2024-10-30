//#include "network/include/SocketOps.h"
#include "network/SocketOps.h"
//#include "network/include/Endian.h"
#include "network/Endian.h"
#include <glog/logging.h>

#include <errno.h>
#include <fcntl.h>
#include <sys/uio.h>
#include <stdio.h>
#include <sys/socket.h>
#include <unistd.h>
#include <cassert>


using namespace network;
/**
 * @brief 将sockaddr_in6* -> sockaddr*
 */
struct sockaddr* socketops::sockaddr_cast(struct sockaddr_in6* addr) {
    return static_cast<struct sockaddr*>(static_cast<void *>(addr));
}

/**
 * @brief 将const sockaddr_in* -> const sockaddr*
 */
const struct sockaddr* socketops::sockaddr_cast(const struct sockaddr_in* addr) {
    return static_cast<const struct sockaddr*>(static_cast<const void*>(addr));
}

/**
 * @brief 将const sockaddr_in6* -> const sockaddr*
 */
const struct sockaddr* socketops::sockaddr_cast(const struct sockaddr_in6* addr) {
    return static_cast<const struct sockaddr*>(static_cast<const void*>(addr));
}

/**
 * @brief 将const sockaddr* -> const sockaddr_in*
 */
const struct sockaddr_in* socketops::sockaddr_in_cast(const struct sockaddr* addr) {
    return static_cast<const struct sockaddr_in*>(static_cast<const void*>(addr));
}

/**
 * @brief 将const sockaddr* -> const sockaddr_in6*
 */
const struct sockaddr_in6* socketops::sockaddr_in6_cast(const struct sockaddr* addr) {
    return static_cast<const struct sockaddr_in6*>(static_cast<const void*>(addr));
}


/**
 * @brief 创建非阻塞的TCP socket
 * @param family 协议族
 * @return socketfd
 */
int socketops::createNoblocking_stop(sa_family_t family) {
    int socketfd = ::socket(family, SOCK_STREAM | SOCK_NONBLOCK | SOCK_CLOEXEC | IPPROTO_TCP, 0);
    if(socketfd >= 0) {
        LOG(INFO) << "socketops:: creatNoBLocking_stop sucess sockfd (SOCK_STREAM | SOCK_NONBLOCK | SOCK_CLOEXEC | IPPROTO_TCP)";
    } else {
        LOG(FATAL) << "socketops:: creatNoBLocking_stop failed sockfd (SOCK_STREAM | SOCK_NONBLOCK | SOCK_CLOEXEC | IPPROTO_TCP)";
    }
    return socketfd;
}

/**
 * @return Return 0 on success, -1 for errors
 */
int socketops::connect(int sockfd, const struct sockaddr* addr) {
    return ::connect(sockfd, addr, static_cast<socklen_t>(sizeof(struct sockaddr_in6)));
}

/**
 * @return void
 */
void socketops::bind_stop(int sockfd, const struct sockaddr* addr) {
    int ret = ::bind(sockfd, addr, static_cast<socklen_t>(sizeof(struct sockaddr_in6)));
}

//未完成
int socketops::accept(int sockfd, struct sockaddr_in6 *addr) {
    socklen_t addrlen = static_cast<socklen_t>(sizeof(*addr));
    //要修改
    int connfd = ::accept4(sockfd, sockaddr_cast(addr), &addrlen, SOCK_NONBLOCK | SOCK_CLOEXEC);

    if(connfd < 0) {
        //保存之前的errno
        int saveerrno = errno;
        LOG(ERROR) << "socketops:: accept failed";
        switch (saveerrno)
        {
            case EAGAIN:
            case ECONNABORTED:
            case EINTR:
            case EPROTO:
            case EPERM:
            case EMFILE:
            {
                errno = saveerrno;
                break;
            }
            case EBADF:
            case EFAULT:
            case EINVAL:
            case ENFILE:
            case ENOBUFS:
            case ENOMEM:
            case ENOTSOCK:
            {
                LOG(FATAL) << "unexpected error of socketops:: accept failed";
                break;
            }
            default:
            {
                LOG(FATAL) << "unkonwn error of socketops:: accept failed";
                break;
            }
        }
            
    }
    return connfd;
}

/**
 * @return void
 */
void socketops::listen_stop(int sockfd) {
    //最大连接值为4096
    int ret = ::listen(sockfd, SOMAXCONN);
    if(ret < 0) {
        LOG(FATAL) << "socketops:: listen failed";
    }
}

/**
 * @return Return the number read, -1 for errors or 0 for EOF
 */
ssize_t socketops::read(int sockfd, void* buf, size_t count) {
    return ::read(sockfd, buf, count);
}

/**
 * @return eturn the number written, or -1.
 */
ssize_t socketops::write(int sockfd, const void* buf, ssize_t count) {
    return ::write(sockfd, buf, count);
}

/**
 * @return 
 */
ssize_t socketops::readv(int sockfd, const struct iovec* iov, int iovcnt) {
    return ::readv(sockfd, iov, iovcnt);
}

/**
 * @return void
 */
void socketops::close(int sockfd) {
    if(::close(sockfd) < 0) {
        LOG(ERROR) << "socketops:: close failed";
    }
}

/**
 * @return void
 */
void socketops::shutdownwrite(int sockfd) {
    if(::shutdown(sockfd, SHUT_WR) < 0) {
        //LOG_SYSERR << "socketops:: shutdownwrite failed";
    }
}

/**
 * @brief ip+port转换为字符串
 */
void socketops::toIpPort(char* buf, ssize_t size, const struct sockaddr* addr) {
    if(addr->sa_family == AF_INET6) {
        //ipv6
        buf[0] = '[';
        //先占用了一个字符
        socketops::toIp(buf + 1, size - 1, addr);
        size_t len = ::strlen(buf);

        const struct sockaddr_in6* addr6 = sockaddr_in6_cast(addr);
        //端口
        uint16_t port = endian::net_to_host_16(addr6->sin6_port);
        assert(size > len);
        //现在buf中已经有了ip，后面需要添加端口
        snprintf(buf + len, size - len, "]:%u", port);
        return;
    }
    //ipv4
    socketops::toIp(buf, size, addr);
    size_t len = ::strlen(buf);
    assert(size > len);
    //端口
    const struct sockaddr_in* addr4 = sockaddr_in_cast(addr);
    uint16_t port = endian::net_to_host_16(addr4->sin_port);
    //现在buf中已经有了ip，后面需要添加端口
    snprintf(buf + len, size - len, ":%u", port);
}

/**
 * @brief 将ip转换为字符串, 牵扯到的函数，详细请看 unix(套接字联网AIPI) 卷一 第三章
 */
void socketops::toIp(char* buf, ssize_t size, const struct sockaddr* addr) {
    //buf -> {......  + ip + port};
    //ipv4
    /*
    #define INET_ADDRSTRLEN 16
    #define INET6_ADDRSTRLEN 46
    */
    if(addr->sa_family == AF_INET) {
        //检查是否超过限制
        assert(size >= INET_ADDRSTRLEN);
        const struct sockaddr_in* addr4 = sockaddr_in_cast(addr);
        ::inet_ntop(AF_INET, &addr4->sin_addr, buf, static_cast<socklen_t>(size));
    } else if(addr->sa_family == AF_INET6) {
        assert(size >= INET6_ADDRSTRLEN);
        const struct sockaddr_in6* addr6 = sockaddr_in6_cast(addr);
        ::inet_ntop(AF_INET6, &addr6->sin6_addr, buf, static_cast<socklen_t>(size));
    }
    //不用返回buf，buf是以指针的形式传递进来的
} 

/**
 * @brief 设置sockaddr_in addr 相关参数 也就是初始化sockaddr_in结构体
 * @param ip ip字符串
 * @param port 端口
 * @param addr sockaddr_in结构体
 */
void socketops::fromIpPort(const char* ip, uint16_t port, struct sockaddr_in* addr) {
    addr->sin_family = AF_INET;
    addr->sin_port = endian::host_to_net_16(port);
    if(::inet_pton(AF_INET, ip, &addr->sin_addr) <= 0) {
        LOG(ERROR) << "socketops:: fromIpPort failed";
    }
}

/**
 * @brief 设置sockaddr_in6 addr 相关参数 也就是初始化sockaddr_in6结构体
 * @param ip ip字符串
 * @param port 端口
 * @param addr sockaddr_in结构体
 */
void socketops::fromIpPort(const char* ip, uint16_t port, struct sockaddr_in6* addr) {
    addr->sin6_family = AF_INET6;
    addr->sin6_port = endian::host_to_net_16(port);
    if(::inet_pton(AF_INET6, ip, &addr->sin6_addr) <= 0) {
        LOG(ERROR) << "socketops:: fromIpPort failed";
    }
}

/**
 * @brief 检查连接是否成功或有错误发生
 */
int socketops::getSocketError(int sockfd) {
    int optionval;
    socklen_t oplen  = static_cast<socklen_t>(sizeof(optionval));
    if(::getsockopt(sockfd, SOL_SOCKET, SO_ERROR, &optionval, &oplen) < 0) {
        LOG(ERROR) << "socketops:: getSocketError failed";
        return errno;
    } else {
        return optionval;
    }
}

/**
 * @brief 获取本地地址 详细参考unix(套接字联网AIPI) 卷一 第四章
 */
struct sockaddr_in6 socketops::getLocalAddr(int sockfd) {
    struct sockaddr_in6 LocalAddr;
    socklen_t addrlen = static_cast<socklen_t>(sizeof(LocalAddr));
    if(::getsockname(sockfd, sockaddr_cast(&LocalAddr), &addrlen) < 0) {
        LOG(ERROR) << "socketops:: getLocalAddr failed";
    }
    return LocalAddr;
}

/**
 * @brief 获取对端地址 详细参考unix(套接字联网AIPI) 卷一 第四章
 */
struct sockaddr_in6 socketops::getPeerAddr(int sockfd) {
    struct sockaddr_in6 PeerAddr;
    socklen_t addrlen = static_cast<socklen_t>(sizeof(PeerAddr));
    if(::getpeername(sockfd, sockaddr_cast(&PeerAddr), &addrlen) < 0) {
        LOG(ERROR) << "socketops:: getPeerAddr failed";
    }
    return PeerAddr;
}
