#pragma once

#include <functional>
#include <string>
#include <vector>
#include <memory>

namespace network {
class EventLoop;
class EventLoopThread;

//线程池类，每个线程都是EventLoopthread，绑定了自己的EventLoop
class EventLoopThreadPool {
public:
 typedef std::function<void(EventLoop*)> threadInitCallback;
 EventLoopThreadPool(EventLoop* baseloop, const std::string& name);
 ~EventLoopThreadPool();
 bool isStarted() const {return started_;}
 const std::string name() const {return name_;}
 
 void serthreadnum(int num) {numThreads_ = num;}
 void start(const threadInitCallback& cb = threadInitCallback());

 
 EventLoop* getNextEventLoop();
 std::vector<EventLoop*> getallEventLoops();
 EventLoop* getEventLoopHash(size_t hashcode);



private:
 EventLoop* baseloop_;//main_EventLoop
 std::string name_;
 bool started_;
 int numThreads_;//线程数量
 int next_;//下一个线程的索引 hashcode
 //std::vector<EventLoopThread*> threads_;//线程池
 std::vector<std::unique_ptr<EventLoopThread*>> threads_;//线程池
 std::vector<EventLoop*> loops_;//EventLoop列表
    //两个相互映射

};
}