#include "network/EventLoopThreadPool.h"

#include <glog/logging.h>
#include "network/EventLoop.h"
#include "network/EventLoopThread.h"

using namespace network;

namespace network {
EventLoopThreadPool::EventLoopThreadPool(EventLoop* baseloop, const std::string& name): 
                                        baseloop_(baseloop), name_(name), started_(false)
                                        numThreads_(0), next_(0){}

EventLoopThreadPool::~EventLoopThreadPool() {
    //不需要deltete baseloop_,
    //涉及到线程的相关知识
    //因为baseloop_是由我们的main线程创建的，在线程栈中，随着main线程的结束，会自动销毁，无需手动销毁
}

void EventLoopThreadPool::start(const threadInitCallback& cb) {
    assert(!started_);
    started_ = true;
    baseloop_->assertInLoopThread();

    for(int i = 0; i < numThreads_; i++) {
        char buf[name_.size() + 32];
        LOG(INFO) << "name = " << name_.c_str();
        EventLoopThread* t = new EventLoopThread(cb, buf);
        /*unique_ptr 转移所有权*/
        threads_.push_back(std::unique_ptr<EventLoopThread>(t));
        loops_.push_back(t->startLoop());
    }
    if(numThreads_ == 0 && cb) {
        cb(baseloop_);
    }
}

EventLoop* EventLoopThreadPool::getNextEventLoop() {
    baseloop_->assertInLoopThread();
    assert(started_);
    EventLoop* loop = baseloop_;//防止返回空指针

    if(!loops_.empty()) {
        loop = loops_[next_];
        next_++;
        if(size_t(next_) > loops_.size()) {
            //回绕回0
            next_ = 0;
        }
    }
    return loop;
}

EventLoop* EventLoopThreadPool::getEventLoopHash(size_t hashcode) {
    baseloop_->assertInLoopThread();
    assert(started_);
    EventLoop* loop = baseloop_;//防止返回空指针

    if(!loops_.empty()) {
        loop = loops_[hashcode % loops_.size()];
    }
    return loop;
}

std::vector<EventLoop*> EventLoopThreadPool::getallEventLoops() {
    baseloop_->assertInLoopThread();
    assert(started_);
    if(!loops_.empty()) {
        return loops_;
    } else {
        //为空，不返回空的列表，返回只有一个baseloop的列表
        return std::vector<EventLoop*>(1, baseloop_);
    }
}
}