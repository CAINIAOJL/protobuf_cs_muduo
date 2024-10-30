#pragma once

#include <atomic>
#include <map>

#include "TcpConnection.h"

namespace network {
class Acceptor;
class EventLoop;
class EventLoopThreadPool;

class TcpServer {
public:
 typedef std::function<void(EventLoop* loop)> ThreadInitCallback;
 enum Option {
    KNoReusePort,
    KReusePort,
 };

 TcpServer(EventLoop* loop, const Inet_address& listenAddr, const std::string& namestring, Option option = KNoReusePort);
 ~TcpServer();

 const std::string& name() const {return name_;}
 const std::string& ipPort() const {return ipPort_;}
 EventLoop* getLoop() const {return loop_;}
 std::shared_ptr<EventLoopThreadPool> getthreadPool() {return threadpool_;}

 void setThreadNum(int numThreads);
 void setThreadInitCallback(const ThreadInitCallback& cb) {
    threadinitcallback_ = cb;
 }

 void start();

 //设置连接回调函数
 void setConnectioncallback(const ConnectionCallback& cb) {
    connectioncallback_ = cb;
 }

 //设置消息回调函数
 void setMessageCallback(const MessageCallback& cb) {
    messagecallback_ = cb;
 }

 //设置写完成回调函数
 void setWriteCompleteCallback(const WriteCompleteCallback& cb) {
    writecompletecallback_ = cb;
 }

private:
 typedef std::map<std::string, TcpConnectionptr> ConnectionMap;
 
 void newConnection(int sockfd, const Inet_address& peerAddr);
 void removeConnection(const TcpConnectionptr& conn);
 void removeConnectionInLoop(const TcpConnectionptr& conn);

 EventLoop* loop_;
 const std::string name_;
 const std::string ipPort_;
 std::unique_ptr<Acceptor> accpetor_;//接收器
 std::shared_ptr<EventLoopThreadPool> threadpool_;//线程池
 ConnectionCallback connectioncallback_;//连接回调函数
 MessageCallback messagecallback_;//消息回调函数
 WriteCompleteCallback writecompletecallback_;//写完成回调函数
 std::atomic<bool> started_;//是否已经启动
 int nxtConnId_;//下一个连接的id
 
 ConnectionMap connections_;//连接集合
 ThreadInitCallback threadinitcallback_;//线程初始化回调函数

};
}