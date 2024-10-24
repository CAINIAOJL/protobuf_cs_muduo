#pragma once

#include <functional>

#include "Channel.h"
#include "Socket.h"

namespace network {
/*Acceptord的前提类*/
class EventLoop;
class Inet_address ;
class Acceptor  {
public:
 /*设置回调函数*/
 typedef std::function<void(int sockfd, const Inet_address&)> NewConnectionCallback;
 Acceptor(EventLoop* Loop, const Inet_address& listen_addr, bool reuseport);
 ~Acceptor();

 setNewConnectioncallback(const NewConnectionCallback& cb) {
    /*设置新连接回调*/
    new_connection_callback_ = cb;
 }

 void listen();
 bool is_listening() const {return listening_;}

private:
 void handle_read();

 EventLoop* loop_;
 /*注意这是服务器监听套接字*/
 /*容易混淆成客户端套接字*/
 Socket listen_fd_;
 Channel* active_cahnnel_;
 bool listening_;
 int idlefd_;
 NewConnectionCallback new_connection_callback_;
}
}



