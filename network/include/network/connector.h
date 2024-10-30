#pragma once

#include "Inet_address.h"

#include <functional>
#include <memory>

namespace network {
class Channel;
class EventLoop;
class Connector {
public:
 typedef std::function<void(int sockfd)> NewConnectionCallback;
 Connector(EventLoop* loop, const Inet_address& server_addr);
 ~Connector();
    
 //开始
 void start();
 //重启
 void restart();
 //停止
 void stop();

 //设置回调函数
 void setNewConnectionCallback(const setNewConnectionCallback& cb) {
    newConnectionCallback = cb;
 }

 const Inet_address& serverAddress() const {return server_addr_;} 
 
 private:
 //三种连接状态
 enum State { KDisconnected, KConnecting, KConnected};
 static const int KMaxRetryDelayMs = 30 * 1000; //最大重试时间
 static const int KInitialRetryDelayMs = 500; //初始重试时间

 void setState(State s) {
    state_ = s;
 }
 void startInLoop();
 void stopInLoop();
 void connect();
 void handleWrite();
 void handleError();
 void connecting(int sockfd);
 void retry();
 int removeAndResetChannel();
 void resetChannel();

 EventLoop* loop_;
 Inet_address server_addr_;
 State state_;
 bool connect_;
 std::unique_ptr<Channel> channel_;
 NewConnectionCallback newConnectionCallback;
 int retry_delay_ms_;
};
}