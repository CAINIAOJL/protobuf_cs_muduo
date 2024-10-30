#pragma once

#include "TcpConnection.h"

#include <mutex>

namespace network {
class Connector;
typedef std::shared_ptr<Connector> Connectorptr;//共享指针

class TcpClient {

public:
 TcpClient(EventLoop* loop, const std::string& name, const Inet_address& serveraddr);
 ~TcpClient();

 void connect();//开启连接
 void disconnect();//断开连接
 void stop();    //停止客户端
 
 EventLoop* getloop() const {return loop_;}
 bool istry() const {return retry_;}
 bool isconnecting() const {return connecting_;}

 const std::string& getname() const {return name_;}

 /*设置回调函数*/
 void setConnectionCallback(ConnectionCallback cb) {
    connectioncallback_ = std::move(cb);
 }

 void setMessageCallback(MessageCallback cb) {
    messagecallback_ = std::move(cb);
 }

 void setWriteCompleteCallback(WriteCompleteCallback cb) {
    writecompletecallback_ = std::move(cb);
 }

private:
 void newConnection(int sockfd);
 void removeConnection(const TcpConnectionptr& conn);

 EventLoop* loop_;
 Connectorptr connector_; //连接器，TcpClient的核心
 const std::string name_;
 ConnectionCallback connectioncallback_;
 MessageCallback messagecallback_;
 WriteCompleteCallback writecompletecallback_;
 bool retry_;
 bool connecting_;
 
 int nxtconnid_;
 mutable std::mutex mutex_;
 TcpConnectionptr connection_; //管理连接
 

};
}

