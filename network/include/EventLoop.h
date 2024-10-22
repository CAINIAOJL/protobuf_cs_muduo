#pragma once
#include "utill.h"

#include <atomic>
#include <functional>
#include <mutex>
#include <vector>

#include <boost/any.hpp>
using namespace network;


class Channel;
class Poller;
namespace network
{
class EventLoop {
public:
 typedef std::function<void()> Functor;
 EventLoop();
 ~EventLoop();
 /*开启循环*/
 void loop();
 void quit();
 
 void runInLoop(Functor cb);
 void queueInLoop(Functor cb);

 size_t queueSize() const;
 int64_t iteration() const {
    return iteration_;
}

 void wakeup();
 void updateChannel(Channel* channel);
 voud removeChannel(Channel* channel);
 bool hasChannel() const;
 
 void assertInLoopThread() {
    if(isInloopthread()) {
        AortNotInLoopThread();
    }
 }
 
 bool isInloopthread() const {
    /*判断当前线程是否为EventLoop所在线程*/
    /*比较线程的id*/
    return threadPid == GetThreadId();
 }
 bool eventHandling() const {
    return eventHanding_;
 }

 void setcontext(const boost::any& context) {
    context_ = context;
 }
 const boost::any& getcontext() const {
    return context_;
 }
 boost::any* mutablecontext() {
    return &context_;
 }

 static EventLoop* getEventLoopofCurrentThread();

private:
 void AortNotInLoopThread();
 void handleRead();
 void dopendingFunctors();
 void debugString();

 typedef std::vector<Channel*> ChannelList;

 bool looping_;
 std::atomic<bool> quit_;
 //意义不大
 std::atomic<bool> eventHanding_;
 std::atomic<bool> callingPendingFunctors_;
 boost::any context_; /*用户自定义的上下文*/

 int64_t iteration_;/*loop迭代次数*/

 /*mutable关键字修饰的成员变量，在const成员函数中被修改，因此需要加锁*/
 /*mutable只能作用在类成员上，指示其数据总是可变的，即使在const成员函数中被修改也不会引起编译错误*/
 mutable std::mutex mutex_;
 pid_t threadPid;
 /*由Linux中的eventfd充当任务*/
 /*事件描述符*/
 int wakeupfd_;/*唤醒fd*/
 std::vector<Functor> pendingFunctors_;/*待执行的函数列表*/

 std::weak_ptr<Poller> poller_;/*延长生命周期*/
 std::unique_ptr<Channel> wakeupChannel_;
 Channel* currentActiveChannel_;/*当前活跃的channel*/
 ChannelList activeChannels_;/*活跃的channel列表*/

}

} // namespace network
