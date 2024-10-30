#include "network/TcpConnection.h"

#include <errno.h>




#include <glog/logging.h>

#include "network/Channel.h"
#include "network/EventLoop.h"
#include "network/Socket.h"
#include "network/SocketOps.h"
#include "network/Buffer.h"

using namespace network;

namespace network {
/**
 * @brief 默认的连接状态回调函数
 */
void defaultConnectionCallback(const TcpConnectionptr& conn) {
    LOG(INFO) << conn->getlocaladdr().Ip_Port_to_string() << " -> " << conn->getpeeraddr().Ip_Port_to_string() << " is " << (conn->connected()? "UP" : "DOWN");
}

/**
 * @brief 默认的消息回调函数
 */
void defaultMessageCallback(const TcpConnectionptr& conn, Buffer* buf) {
    buf->retrieveAll();
}

TcpConnection::TcpConnection(EventLoop* loop, const std::string& name, int sockfd, const Inet_address& localaddr, const Inet_address& peeraddr) :
                            loop_(CHECK_NOTNULL(loop)),
                            name_(name),
                            state_(KConnecting),
                            reading_(true),
                            socket_(new Socket(sockfd)),
                            channel_(new Channel(loop, sockfd)),
                            localaddr_(localaddr),
                            peeraddr_(peeraddr){
    channel_->setErrorEventCallback(std::bind(&TcpConnection::handleError, this));
    channel_->setReadEventCallback(std::bind(&TcpConnection::handleRead, this));
    channel_->setWriteEventCallback(std::bind(&TcpConnection::handleWrite, this));
    channel_->setCloseCallback(std::bind(&TcpConnection::handleClose, this));

    LOG(INFO) << "TcpConnection::ctor[" << name_ << "] at " << this << " fd=" << sockfd << " local=" << localaddr.Ip_Port_to_string() << " peer=" << peeraddr.Ip_Port_to_string();
    socket_->setKeepAlive(true);
}

TcpConnection::~TcpConnection() {
    LOG(INFO) << "TcpConnection::dtor[" << name_ << "] at " << this << " fd=" << channel_->fd() << " state=" << stateToString();
    assert(state_ == KDisconnected);
}

bool TcpConnection::getTcpInfo(struct tcp_info* info) const {
    return socket_->getTcpInfo(info);
}

std::string TcpConnection::stateToString() const {
    char buf[1024];
    buf[0] = '\0';
    socket_->getTcpInfoString(buf, sizeof(buf));
    return buf;
}

/**
 * @brief 发送数据，如果在loop线程中，则直接发送，否则将数据放入loop的队列中
 */
void TcpConnection::send(Buffer* buf) {
    //判断是否连接
    if(state_ == KConnected) {
        if(loop_->isInloopthread()) {
            sendInLoop(buf->peek(), buf->readableBytes());
            buf->retrieveAll();
        } else {
            std::function<void(const std::string&)> fp =
                [this](const std::string& msg) {
                    this->sendInLoop(msg);
                };
                //buf->retrieveAllAsString()作为参数传递给fp函数
                loop_->runInLoop(sd::bind(fp, buf->retrieveAllAsString()));
        }
    }
}

/**
 * @brief 发送数据，(在loop线程中发送)
 */
void TcpConnection::sendInLoop(const void* message, size_t len) {
    loop_->assertInLoopThread();
    ssize_t nwrote = 0;
    size_t remaining = len;
    bool faultError = false;
    if(state_ == KDisconnected) {
        //没有连接，直接返回
        LOG(INFO) << "disconnected, give up writing";
        return;
    }

    //channel不可写，并且用户的outputbuffer要发送的数据为0
    if(!channel_->isWriting() && outputbuffer_.readableBytes() == 0) {
        nwrote = socketops::write(channel_->fd(), message, len);
        if(nwrote >= 0) {
            remaining = len - nwrote;
            if(remaining == 0 && writecompletecallback) {
                //shared_from_this实现完美转发，保证回调函数的this指针为TcpConnection的智能指针
                loop_->queueInLoop(std::bind(writecompletecallback, shared_from_this()));
            }
        } else {
            //write写失败，提示错误，分析错误
            nwrote = 0;
            if(errno != EWOULDBLOCK) {
                LOG(ERROR) << "TcpConnection::sendInLoop write error";
                //EPIPE write to a socket which is already accepted rst or closed by peer
                //ECONNRESET connection reset by peer
                if(errno == EPIPE || errno == ECONNRESET) {
                    //如果发生了上述的错误，说明连接断开
                    faultError = true;
                }
            }
        }
    }
    
    //向用户缓冲区写入未发送完的数据
    if(!faultError && remaining > 0) {
        size_t oldlen = outputbuffer_.readableBytes();
        //remaining = len - nwrote;
        outputbuffer_.append(static_cast<const char*>(message) + nwrote, remaining);
        //channel不可写，则开启可写
        if(!channel_->isWriting()) {
            channel_->enableWriting();
        }
    }
}

/**
 * @brief 关闭套接字写端，回调至loop线程
 */
void TcpConnection::shutdown() {
    if(state_ == KConnected) {
        setstate(KDisconnecting);
        loop_->runInLoop(std::bind(&TcpConnection::shutdownInLoop, this));
    }
}

/**
 * @brief 关闭套接字的写端 （在loop线程中关闭）
 */
void TcpConnection::shutdownInLoop() {
    loop_->assertInLoopThread();
    if(!channel_->isWriting()) {
        socket_->ShutdownWrite();
    }
}

/**
 * @brief 强制关闭连接
 */
void TcpConnection::forceClose() {
    if(state_ == KConnected || state_ ==  KConnecting) {
        setstate(KDisconnecting);
        loop_->queueInLoop(std::bind(&TcpConnection::forceCloseInLoop, shared_from_this()));
    }
}

/**
 * @brief 强制关闭连接 （在loop线程中关闭）
 */
void TcpConnection::forceCloseInLoop() {
    loop_->assertInLoopThread();
    if(state_ == KConnected || state_ == KConnecting) {
        handleClose();
    }
}

const char* TcpConnection::stateToString() const {
    switch(state_) {
        case KConnected:
            return "KConnected";
        case KConnecting:
            return "KConnecting";
        case KDisconnecting:
            return "KDisconnecting";
        case KDisconnected:
            return "KDisconnected";
        default:
            return "unknown state";
    }
}

void TcpConnection::setTcpNoDelay(bool on) {
    socket_->setTcpNoDelay(on);
}

/**
 * @brief 开始读取数据
 */
void TcpConnection::startRead() {
    loop_->runInLoop(std::bind(&TcpConnection::startReadInLoop, this));
}

/**
 * 开始读取数据，（在loop线程中）
 */
void TcpConnection::startReadInLoop() {
    loop_->assertInLoopThread();
    if(!reading_ || !channel_->isReading()) {
        channel_->enableReading();
        reading_ = true;
    }
}

/**
 * @brief 停止读取数据
 */
void TcpConnection::stopRead() {
    loop_->runInLoop(std::bind(&TcpConnection::stopReadInLoop, this));
}

/**
 * @brief 停止读取数据 （在loop线程中）
 */
void TcpConnection::stopReadInLoop() {
    loop_->assertInLoopThread();
    if(reading_ && channel_->isReading()) {
        channel_->disenableReading();
        reading_ = false;
    }
}

/**
 * @brief 连接建立
 */
void TcpConnection::connectEstablished() {
    loop_->assertInLoopThread();
    assert(state_ == KConnecting);
    setstate(KConnected);
    channel_->enableReading();
    connectioncallback(shared_from_this());
}

/**
 * @brief 连接断开，channel从loop中移除
 */
void TcpConnection::connectDestroyed() {
    loop_->assertInLoopThread();
    if(state_ == KConnected) {
        setstate(KDisconnected);
        channel_->disableall();

        connectioncallback(shared_from_this());
    }
    channel_->remove();
}

/**
 * @brief 处理写事件，通过分散读套接字，将用户数据写入缓冲区，详细阅读readfd函数
 */
void TcpConnection::handleRead() {
    loop_->assertInLoopThread();
    int savedErrno = 0;
    ssize_t n = inputbuffer_.readFd(channel_->fd(), &savedErrno);
    if(n > 0) {
        messagecallback(shared_from_this(), &inputbuffer_);
    } else if(n == 0) {
        handleClose();
    } else if(n < 0) {
        errno = savedErrno;
        LOG(ERROR) << "TcpConnection::handleRead read error";
        handleError();
    }
}

/**
 * @brief 处理写事件，将用户缓冲区数据写入套接字，发生在write()函数调用后，多出来的未发送的数据，这些数据被写在outputbuffer中，将outputbuffer中readableBytes()数据写入套接字
 */
void TcpConnection::handleWrite() {
    loop_->assertInLoopThread();
    if(channel_->isWriting()) {
        ssize_t n = ::write(channel_->fd(), outputbuffer_.peek(), outputbuffer_.readableBytes());
    
        if(n > 0) {
            //实际上，数据被发送，缓冲区取出相应数据
            outputbuffer_.retrieve(n);
            //触发低水位
            if(outputbuffer_.readableBytes() == 0) {
                channel_->disableWriting();
                if(writecompletecallback) {
                    loop_->queueInLoop(std::bind(&TcpConnection::writecompletecallback, shared_from_this()));
                }
                if(state_ == KDisconnecting) {
                    shutdownInLoop();
                }
            }
        } else {
            LOG(ERROR) << "TcpConnection::handleWrite write error";
        }
    } else {
        LOG(INFO) << "Connection fd = " << channel_->fd() << "is down, no more writing";
    }
}

/**
 * @brief 处理错误事件
 */
void TcpConnection::handleClose() {
    loop_->assertInLoopThread();
    LOG(INFO) << "fd =" << channel_->fd() << "state = " << stateToString();
    assert(state_ == KConnected || state_ == KDisconnecting);
    setstate(KDisconnected);
    channel_->disableall();

    TcpConnectionptr guardThis(shared_from_this());
    connectioncallback(guardThis);
    closecallback(guardThis);
}

/**
 * @brief 处理错误事件
 */
void TcpConnection::handleError() {
    int err = socketops::getSocketError(channel_->fd());
    LOG(ERROR) << "TcpConnection::handleError [" << name_ << "] - SO_ERROR = " << err;
}
}