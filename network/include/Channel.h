#pragma once

#include <sys/epoll.h>
#include <functional>
#include <atomic>/*c++11 原子类中*/
/*在这个protbuf_rpc中，不涉及多线程*/
#include <memory>/*智能指针*/
namespace network
{
class EventLoop;
class Channel {
public:    
    enum EventTypes {
        NoneEvent = 0,
        ReadEvent,
        WriteEvernt,
        ErrorEvent
    };
 /*事件回调函数*/
 typedef std::function<void()> EventCallback;
 explicit Channel(EventLoop* loop, int fd);   
 ~Channel();
 
 /*处理事件*/
 void handleEvent();
 
 /*注册事件:
 读事件；
 写事件；
 错误事件；*/
 void setReadEventCallback(EventCallback cd) {readCallback_ = std::move(cb);}
 void setWriteEventCallback(EventCallback cb) {writeCallback_ = std::move(cb);}
 void setErrorEventCallback(EventCallback cb) {errorCallback_ = std::move(cb);}
 
 int fd() const {return fd_;}
 int event() const {return events_;}
 int index() const {return index_;}/*for poller*/
 EventLoop* ownerLoop() const {return loop_;}

 void setEvent(int event) {events_ |= event;}
 void setRevent(int revent) {revents_ |= revent;}
 void setIndex(int index) {index_ = index;.}/*for poller*/
 bool isNoneEvent() const {return events_ == EventTypes::NoneEvent;}

 void enableReading() {
    events_ |= EventTypes::ReadEvent;
    update();/*转到EventLoop中*/
 }
 void disenableReading() {
    events_ &= ~EventTypes::ReadEvent;
    update();
 }
 
 void enableWriting() {
    events_ |= EventTypes::WriteEvernt;
    update();/*转到EventLoop中*/
 }
 void disableWriting() {
    events_ &= ~EventTypes::WriteEvernt;
    update();
 }

 void disableall() {
    events_ = EventTypes::NoneEvent;
    update();
 }
 
 bool isReading() const {return events_ & EventTypes::ReadEvent;}
 bool isWriting() const {return events_ & EventTypes::WriteEvernt;}

 void remove();

 private:
 void update(); /*转到EventLoop中*/
 //未用到
 //static const int KNoneEvent; /*无事件*/
 //static const int KReadEvent; /*读事件*/
 //static const int KWriteEvent; /*写事件*/

  EventLoop* loop_; /*属于EventLoop中*/
  int fd_;/*文件描述符*/ /*这里的套接字其实是临时的, 真正的监听套接字在Acceptor中*/
  int events_; /*事件类型*/
  int revents_; /*实际发生的事件*/
  int index_; 
  //boost::weak_ptr<void> tie_; 不存在这个标志，延长对象的生命周期，在多线程中使用
  std::atomic<bool> eventHandling_; /*原子类型，是否正在处理事件*/
  std::atomic<bool> addedToLoop_; /*原子类型，是否已经加入到EventLoop中*/
  EventCallback readCallback_;
  EventCallback writeCallback_;
  EventCallback errorCallback_;
  EventCallback closeCallback_; /*关闭事件回调函数*/
}
} // namespace network
