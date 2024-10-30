#include "network/TcpClient.h"
#include <stdio.h>
#include <cassert>

#include <glog/logging.h>

#include "network/connector.h"
#include "network/EventLoop.h"
#include "network/SocketOps.h"

using namespace network;

namespace network {
namespace detail {
void removeConnection(EventLoop* loop, const TcpConnectionptr& conn) {
    loop->queueInLoop(std::bind(&TcpConnection::connectDestroyed, conn));
}

void removeConnector(const Connectorptr& connector) {
    //
}
}

/**
 * @brief TcpClient构造函数，当TcpClient对象被创建时，connector_对象回调TcpClient对象中的newConnection()函数
 * 
 */
TcpClient::TcpClient(EventLoop* loop, const std::string& name, const Inet_address& serveraddr) :
                    loop_(CHECK_NOTNULL(loop)),
                    name_(name),
                    connectioncallback_(defaultConnectionCallback),
                    messagecallback_(defaultMessageCallback),
                    retry_(false),
                    connecting_(true),
                    connector_(new Connector(loop, serveraddr)),
                    nxtconnid(1) {
    //设置回调函数
    connector_->setNewConnectionCallback(std::bind(&TcpClient::newConnection, this, _1));
    LOG(INFO) << "TcpClient::TcpClient [" << name_ << "] - connector = " << get_pointer(connector_);
}

TcpClient::~TcpClient() {
    LOG(INFO) << "TcpClient::~TcpClient [" << name_ << "] - connector = " << get_pointer(connector_);

    TcpConnectionptr conn;
    bool isunique = false;
    //线程安全
    {
        std::unique_lock<std::mutex> locker(mutex_);
        isunique = connection_.unique();
        conn = connection_;
    }

    if(conn) {
        assert(loop_ == conn->getloop());
        CloseCallback cb = std::bind(&detail::removeConnection, loop_, conn);
        loop_->runInLoop(std::bind(&TcpConnection::setClosecallback, conn, cb));
        //说明connection_使用者为唯一，可以安全删除
        if(isunique) {
            conn->forceClose();//直接关闭连接
        } else {
            //不是一个对象共享指针
            connector_->stop();
        }
    }
}

/**
 * TcpClient实例中，connector是连接器，是TcpClient的核心，负责管理连接的创建和销毁，TcpClient通过connector来创建连接
 *
 */
void TcpClient::connect() {
    LOG(INFO) << "TcpClient::connect [" << name_ << "] - connecting to " << connector_->serverAddress().Ip_Port_to_string();

    connecting_ = true;
    connector_->start();
}

/**
 * @brief 客户端关闭写端，
 */
void TcpClient::disconnect() {
    connecting_ = false;
    {
        std::unique_lock<std::mutex> locker(mutex_);
        if (connection_) {
            connection_->shutdown();//客户端关闭写端，
        }
    }
}

/**
 * @brief TcpClient停止, 实际是connector停止
 */
void TcpClient::stop() {
    connecting_ = false;
    connector_->stop();
}

/**
 * @brief 绑定在Connector上的回调函数，当有新连接时，会调用这个函数，创建新的TcpConnection对象，并设置回调函数
 */
void TcpClient::newConnection(int sockfd) {
    //这个sockfd由谁去连接
    //这个sockfd是客户端的套接字
    loop_->assertInLoopThread();
    Inet_address peerAddr(socketops::getPeerAddr(sockfd));
    char buf[32];
    snprintf(buf, sizeof(buf), "%s#%d", peerAddr.Ip_Port_to_string().c_str(), nxtconnid_);
    nxtconnid_++;
    std::string connname = name_ + buf;

    Inet_address localAddr(socketops::getLocalAddr(sockfd));

    //创建TcpConnectionptr智能指针
    TcpConnectionptr conn(new TcpConnection(loop_, connname, sockfd, localAddr, peerAddr));

    conn->setConnectionCallback(connectioncallback_);
    conn->setMessageCallback(messagecallback_);
    conn->setWriteCompleteCallback(writecompletecallback_);
    conn->setClosecallback(std::bind(&TcpClient::removeConnection, this, _1));
    {
        std::unique_lock<std::mutex> locker(mutex_);
        //shared_ptr指针赋值，绑定的对象计数器加一
        connection_ = conn;//这里赋值，不会导致原来的conn被销毁
        //connection_计数器加1，conn计数器加一，connection_的生命周期大于conn，conn被销毁时，connection_计数器减1，当connection_计数器为0时，说明connection_已经没有引用，可以安全删除
    }
    conn->connectEstablished();
}

/**
 * @brief 移除连接
 */
void TcpClient::removeConnection(const TcpConnectionptr& conn) {
    loop_->assertInLoopThread();
    assert(loop_ == conn->getloop());
    {
        std::unique_lock<std::mutex> locker(mutex_);
        assert(connection_ == conn);
        connection_.reset();
    }

    //在loop中执行，绑定了removeConnection的回调函数，会在loop线程中执行
    loop_->queueInLoop(std::bind(&TcpConnection::connectDestroyed, conn));
    if(retry_ && connecting_) {
       LOG(INFO) << "TcpClient::connect[]" << name_ << "] - reconnecting to" << connector_->serverAddress().Ip_Port_to_string();
        connector_->restart();
    }
}
}
