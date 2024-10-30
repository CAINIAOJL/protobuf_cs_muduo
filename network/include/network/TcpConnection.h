#pragma once

#include "Inet_address.h"
#include "Buffer.h"
#include "Callbacks.h"
#include <boost/any.hpp>
#include <memory>
//#include <netinet/tcp.h>
struct tcp_info;

namespace network {
class Channel;
class EventLoop;
class Socket;

//enable_shared_from_this 实现完美转发 ---》 Effective modern C++ 
class TcpConnection: public std::enable_shared_from_this<TcpConnection> {
public:
 TcpConnection(EventLoop* loop, const std::string& name, int sockfd, const Inet_address& localaddr, const Inet_address& peeraddr);
 ~TcpConnection();

 EventLoop* getloop() const {return loop_;}
 const std::string& getname() const {return name_;}
 const Inet_address& getlocaladdr() const {return localaddr;}
 const Inet_address& getpeeraddr() const {return peeraddr;}
 //是否正在读
 bool isreading() const {return reading_;}
 //是否已经连接
 bool isconnected() const {return state_ == KConnected;}
 //是否断开连接
 bool isdisconnected() const {return state_ == KDisconnected;}
 
 bool getTcpInfo(struct tcp_info* info) const;

 std::string getIcpInfoString() const;
 
 void send(Buffer* message);
 void shutdown();

 void forceClose();
 //没有实现
 void forceCloseWithDelay(double seconds);

 void setTcpNoDelay(bool on);

 void startRead();
 void stopRead();
 bool isreading() const {return reading_;}

 void setContext(const boost::any& context) {
    context_ = context;
 } 

 const boost::any& getContext() const {
    return context_;
 }

 boost::any* getMutableContext() {
    return &context_;
 }

 //设置回调
 void setConnectionCallback(const ConnectionCallback& cb) {
    connectioncallback = cb;
 }

 void setMessageCallback(const MessageCallback& cb) {
    messagecallback = cb;
 }

 void setWriteCompleteCallback(const WriteCompleteCallback& cb) {
    writecompletecallback = cb;
 }

 void setClosecallback(const CloseCallback& cb) {
    closecallback = cb;
 }

 Buffer* getInputBuffer() {
    return &inputbuffer_;
 }

 Buffer* getOutputBuffer() {
    return &outputbuffer_;
 }

 void connectEstablished();
 void connectDestroyed();

private:

 enum StateE {KDisconnected, KConnecting, KConnected, KDisconnecting};
 //私有函数，外部不可访问
 void handleRead();
 void handleWrite();
 void handleError();
 void handleClose();

 void sendInLoop(const void* message, size_t len);
 void sendInLoop(const std::string& message);
 void startReadInLoop();
 void stopReadInLoop();

 void shutdownInLoop();
 void forceCloseInLoop();
 
 void setState(StateE s) {state_ = s;}
 const char* stateToString() const;
 void startReadInLoop();
 void stopReadInLoop();
 
 EventLoop* loop_;
 const std::string name_;
 StateE state_;
 bool reading_;
 //用unique_ptr管理Socket与Channel，避免出现野指针
 std::unique_ptr<Socket> socket_;
 std::unique_ptr<Channel> channel_;
 const Inet_address localaddr_;//本端地址
 const Inet_address peeraddr_;//对端地址
 //连接回调
 ConnectionCallback connectioncallback;
 //消息回调函数
 MessageCallback messagecallback;
 //关闭回调函数
 CloseCallback closecallback;
 //低水位回调函数
 WriteCompleteCallback writecompletecallback;
 //高水位回调函数
 HighWaterMarkCallback hughwatermarkcallback;

 Buffer inputbuffer_;//内核缓冲区
 Buffer outputbuffer_;//用户缓冲区
 boost::any context_;//用户自定义内容

};
}

