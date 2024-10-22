#include "Inet_address.h"
#include "SocketOps.h"
#include "Socket.h"

#include <glog/logging.h>
#include <stdio.h>
#include <netinet/in.h>
#include <netinet/tcp.h>

using namespace network;
namespace network {

Socket::~Socket(){
    socketops::close(sockfd_);
}

/**
 * @return true on success, false on failure.
 */
bool Socket::getTcpInfo(struct tcp_info* info) const {
    socklen_t len = sizeof(*info);
    memset(info, 0, len);
    /*通过getsockopt获取TCP信息*/
    return ::getsockopt(sockfd_, SOL_TCP, TCP_INFO, info, &len) == 0;
}

bool Socket::setTcpNoDelay(char* buf, int len) const {
    struct tcp_info info;
    bool ok = getTcpInfo(&info);
    if (ok) {
        snprintf(
            buf, len,
            "unrecovered=%u "
            "rto=%u ato=%u snd_mss=%u rcv_mss=%u "
            "lost=%u retrans=%u rtt=%u rttvar=%u "
            "sshthresh=%u cwnd=%u total_retrans=%u",
            tcpi.tcpi_retransmits,  // Number of unrecovered [RTO] timeouts
            tcpi.tcpi_rto,          // Retransmit timeout in usec
            tcpi.tcpi_ato,          // Predicted tick of soft clock in usec
            tcpi.tcpi_snd_mss, tcpi.tcpi_rcv_mss,
            tcpi.tcpi_lost,     // Lost packets
            tcpi.tcpi_retrans,  // Retransmitted packets out
            tcpi.tcpi_rtt,      // Smoothed round trip time in usec
            tcpi.tcpi_rttvar,   // Medium deviation
            tcpi.tcpi_snd_ssthresh, tcpi.tcpi_snd_cwnd,
            tcpi.tcpi_total_retrans);  // Total retransmits for entire connection
    }
    return ok;
}

void Socket::BindAddress(const Inet_address& localaddr) {
    int ret = ::bind(sockfd_, localaddr.getSockAddr(), static_cast<socklen_t>(sizeof(struct sockaddr_in6)));
    if(ret < 0) {
        LOG(FATAL) << "bind error, errno=" << errno;
    }
}

void Socket::Listen() {
    int ret = ::listen(sockfd_, SOMAXCONN);
    if(ret < 0) {
        LOG(FATAL) << "listen error, errno=" << errno;
    }
}

int Socket::Accept(Inet_address* perradddr) {
    struct sockaddr_in6 addr;
    memset(&addr, 0, sizeof(addr));
    socklen_t addrlen = sizeof(addr);
    int connfd = ::accept(sockfd_, reinterpret_cast<sockaddr_in6*>(&addr), &addrlen);
    if(connfd < 0) {
        LOG(FATAL) << "accept error, errno=" << errno;
    }
    if(connfd >= 0) {
        /*设置Inet_address对象的addr*/
        perradddr->setSockAddrInet6(addr);
    } 
    return connfd;
}

void Socket::ShutdownWrite() {
    if(::shutdown(sockfd_, SHUT_WR) < 0) {
        LOG(ERROR) << "shutdown write error, errno=" << errno;
    }
}

void Socket::setTcpNoDelay(bool on) {
    int optval = on ? 1 : 0;
    ::setsockopt(sockfd_, IPPROTO_TCP, TCP_NODELAY, &optval, static_cast<socklen_t>(optval));
}

void Socket::setReuseAddr(bool on) {
    int optval = on ? 1 : 0;
    ::setsockopt(sockfd_, SOL_SOCKET, SO_REUSEADDR, &optval, static_casts<socklen_t>(optval));
}

void Socket::setReusePort(bool on) {
#ifdef SO_REUSEPORT
    int optval = on ? 1 : 0;
    
   int ret = ::setsockopt(sockfd_, SOL_SOCKET, SO_REUSEPORT, &optval, static_cast<socklen_t>(optval));
    if(ret < 0 && on) {
        LOG(ERROR) << "set reuse port error, errno=" << errno;
    }
#else 
    if(on) {
        LOG(ERROR) << "SO_REUSEPORT not supported";
    }
#endif
}

void Socket::setKeepAlive(bool on) {
    int optval = on ? 1 : 0;
    ::setsockopt(sockfd_, SOL_SOCKET, SO_KEEPALIVE, &optval, static_cast<socklen_t>(optval));
}

}

