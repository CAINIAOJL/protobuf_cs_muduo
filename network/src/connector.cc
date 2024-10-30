#include "network/connector.h"
#include "network/EventLoop.h"
#include "network/Channel.h"
#include "network/SocketOps.h"

#include <glog/logging.h>

//问题，有了TcpConnection，为什么还要有Connector？

using namespace network;

namespace network {
Connector::Connector(EventLoop* loop, const Inet_address& server_addr):
                    loop_(loop), server_addr_(server_addr), state_(KDisconnected),
                    connect_(false), retry_delay_ms_(KInitialRetryDelayMs) {
    LOG(INFO) << "Connector::ctor[" << this << "]"

}


Connector::~Connector() {
    log(INFO) << "Connector::dtor[" << this << "]";
}

void Connector::start() {
    connect_ = true;
    LOG(INFO) << "Connector::start[" << this << "]";
    loop_->runInLoop(std::bind(&Connector::startInLoop, this));
}

void Connector::startInLoop() {
    loop_->assertInLoopThread();
    assert(state_ == KDisconnected);
    if(connect_) {
        //调用start函数，connect_为true
        connect();
    } else {
        LOG(INFO) << "do not connect in connector";
    }   
}

void Connector::stop() {
    connect_ = false;
    loop_->queueInLoop(std::bind(&Connector::stopInLoop, this));
}

void Connector::stopInLoop() {
    loop_->assertInLoopThread();
    if(state_ == KConnecting) {
        setstate(KDisconnected);
        //暂且没搞懂什么作用
        int sockfd = removeAndResetChannel();
        retry(sockfd);
    }
}

void Connector::connect() {
    //创建非阻塞的sockfd
    int sockfd = socketops::createNoblocking_stop(server_addr_.family_4());
    //用server_addr_绑定sockfd，调用connect函数
    int ret = socketops::connect(sockfd, server_addr_.getSockAddr());
    //保存错误码
    int saveerror = (ret == 0) ? 0 : errno;

    switch(saveerror) {
        case 0:
        case EINPROGRESS://非阻塞或异步操作已经在进行
        case EINTR://信号中断
        case EISCONN://socket已经连接
            connecting(sockfd);
            break;
        case EAGAIN://重新尝试
        case EADDRNOTAVAIL://地址不可用
        case ECONNREFUSED://连接被拒绝
        case ENETUNREACH://网络不可达
        case ETIMEDOUT://超时
            retry(sockfd);
            break;
        case EACCES://权限不足
        case EPERM://操作不允许
        case EAFNOSUPPORT://地址族不支持
        case EALREADY://非阻塞或异步操作已经在进行
        case EBADF://文件描述符非法
        case EFAULT://故障
        case ENOTSOCK://socket不是一个socket
        case EPROTONOSUPPORT://协议不支持
            LOG(ERROR) << "Connector::connect error in sockfd =" << saveerror;
            ::close(sockfd);//关闭sockfd
            break;

        default:
            LOG(ERROR) << "Connector::connect unknow error in sockfd =" << saveerror;
            ::close(sockfd);//关闭sockfd
            break;
    }
}


void Connector::restart() {
    loop_->assertInLoopThread();
    connect_ = true;
    retry_delay_ms_ = KInitialRetryDelayMs;
    setstate(KDisconnected);
    startInLoop();
}

void Connector::connecting(int sockfd) {
    setstate(KConnecting);//正在连接
    assert(!channel_);
    channel_.reset(new Channel(loop_, sockfd));
    //设置Channel的读与错误事件回调函数
    channel_->setWriteEventCallback(std::bind(&Connector::handleWrite, this));
    channel_->setErrorEventCallback(std::bind(&Connector::handleError, this));
    channel_->enableWriting();
}

int Connector::removeAndResetChannel() {
    channel_->disableall();//关闭功能
    channel_->remove();
    int sockfd = channel_->fd();
    //放入loop_的队列中，等待处理
    loop_->queueInLoop(std::bind(&Connector::resetChannel, this));
    return sockfd;
}

void Connector::resetChannel() {
    //独占指针置空
    channel_.reset();
}

void Connector::handleWrite() {
    LOG(INFO) << "Connector::handleWrite[" << this << "]";
    //处理写事件
    //正在连接，出现错误
    if(state_ == KConnecting) {
        int sockfd = removeAndResetChannel();
        int err = socketops::getSocketError(sockfd);
        if(err) {
            LOG(ERROR) << "Connector::handleWrite[" << this << "] - SO_ERROR = " << err;
            retry(sockfd);
        } else if(socketops::isSelfConnect(sockfd)) {
            LOG(INFO) << "Connector::handleWrite[" << this << "] - self connect";
            retry(sockfd);
        } else {
            //主要工作场景
            setstate(KConnected);
            if(connect_) {
                //连接成功，调用回调函数，处理事务
                newConnectionCallback(sockfd);
            } else {
                socketops::close(sockfd);
            }
        }
    } else{
        assert(state_ == KDisconnected);
    }
}

void Connector::handleError() {
    LOG(ERROR) << "Connector::handleError[" << this << "]";
    if(state_ == KConnecting) {
        int sockfd = removeAndResetChannel();
        int err = socketops::getSocketError(sockfd);
        LOG(ERROR) << "Connector::handleError[" << this << "] - SO_ERROR = " << err;
        retry(sockfd);
    }
}

void Connector::retry(int sockfd) {
    socketops::close(sockfd);
    if(connect_) {
        LOG(INFO) << "Connector::retry[" << this << "] - retry to connect in " << retry_delay_ms_ << " ms";
        // loop_->runAfter(retryDelayMs_ / 1000.0,
        //            std::bind(&Connector::startInLoop, shared_from_this()));
        // retryDelayMs_ = std::min(retryDelayMs_ * 2, kMaxRetryDelayMs);
    } else {
        LOG(INFO) << "Connector::retry[" << this << "] - connector disabled, give up retry";
    }
}
}