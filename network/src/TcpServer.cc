#include "network/TcpServer.h"

#include <stdio.h>
#include <cassert>

#include <glog/logging.h>

#include "network/Acceptor.h"
#include "network/EventLoop.h"
#include "network/EventLoopThreadPool.h"
#include "network/SocketOps.h"
//#include "Inet_address.h"

using namespace network;
namespace network {

/**
 * @brief TcpServer构造函数,在TcpServer类中，完成对acceptor的创建，并设置回调函数
 */
TcpServer::TcpServer(EventLoop* loop, const Inet_address& listenAddr, const std::string& namestring, Option option):
                    loop_(CHECK_NOTNULL(loop)),
                    ipPort_(listenAddr.Ip_Port_to_string()),
                    name_(namestring),
                    accpetor_(new Acceptor(loop, listenAddr, option = KReusePort)),
                    threadpool_(new EventLoopThreadPool(loop, name_)),
                    connectioncallback_(defaultConnectionCallback),
                    messagecallback_(defaultMessageCallback),
                    nxtConnId_(1) {
                        //设置回调函数
    accpetor_->setNewConnectioncallback(std::bind(&TcpServer::newConnection, this, _1, _2));
}

/**
 * @brief TcpServer析构函数,在TcpServer类中，完成对acceptor的关闭，并关闭所有连接
 */
TcpServer::~TcpServer() {
    loop_->assertInLoopThread();
    LOG(INFO) << "TcpServer::~TcpServer [" << name_ << "] destructing";
    started_ = false;
    //server服务器析构，所有连接关闭
    for(auto& item : connections_) {
        TcpConnectionptr conn(item.second);
        item.second.reset();
        conn->getloop()->runInLoop(std::bind(&TcpConnection::connectDestroyed, conn));
    }
}

/**
 * @brief 设置线程池中线程数
 */
void TcpServer::setThreadNum(int numThreads) {
    assert(numThreads >= 0);
    threadpool_->serthreadnum(numThreads);
}

/**
 * @brief 服务器开始启动，acceptor开始监听
 */
void TcpServer::start() {
    threadpool_->start(threadinitcallback_);

    assert(!accpetor_->is_listening());
    loop_->runInLoop(std::bind(&Acceptor::listen, get_pointer(accpetor_)));
}

/**
 * @brief 服务器有新的连接
 */
void TcpServer::newConnection(int sockfd, const Inet_address& peerAddr) {
    loop_->assertInLoopThread();
    //给出subloop
    EventLoop* ioloop = threadpool_->getNextEventLoop();
    char buf[64];
    snprintf(buf, sizeof(buf), "-%s#%d", ipPort_.c_str(), nxtConnId_);
    nxtConnId_++;
    std::string connname = name_ + buf;
    LOG(INFO) << "TcpServer::newConnection [" << name_ << "] - new connection [" << connname << "] from " << peerAddr.Ip_Port_to_string();
    Inet_address localAddr(socketops::getLocalAddr(sockfd));
    //新的连接，分配给线程池中的线程
    TcpConnectionptr conn (new TcpConnection(ioloop, connname, sockfd, localAddr, peerAddr));
    connections_[connname] = conn;
    conn->setConnectionCallback(connectioncallback_);
    conn->setMessageCallback(messagecallback_);
    conn->setWriteCompleteCallback(writecompletecallback_);
    conn->setClosecallback(std::bind(&TcpServer::removeConnection, this, _1));
    ioloop->runInLoop(std::bind(&TcpConnection::connectEstablished, conn));
}

/**
 * @brief 移除连接
 */
void TcpServer::removeConnection(const TcpConnectionptr& conn) {
    loop_->runInLoop(std::bind(&TcpServer::removeConnectionInLoop, this, conn));
}

/**
 * @brief 移除连接, 在loop线程中
 */
void TcpServer::removeConnectionInLoop(const TcpConnectionptr& conn) {
    loop_->assertInLoopThread();
    LOG(INFO) << "TcpServer::removeConnection [" << name_ << "] - connection " << conn->getname();
    size_t n = connections_.erase(conn->getname());
    (void)n;
    assert(n == 1);
    EventLoop* ioloop = conn->getloop();
    ioloop->queueInLoop(std::bind(&TcpConnection::connectDestroyed, conn));
}
}