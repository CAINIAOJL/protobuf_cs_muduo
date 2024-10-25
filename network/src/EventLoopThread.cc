#include "EventLoopThread.h"
#include "EventLoop.h"
#include <memory>
using namespace network;

namespace network {

EventLoopThread::EventLoopThread(const ThreadInitCallback& cb,const std::string& name):
                    loop_(nullptr), isStarting_(false), callback_(cb), mutex_(nullptr){}

EventLoopThread::~EventLoopThread() {
    isStarting_ = false;
    if(loop_ != nullptr) {
        loop_->quit();//关闭事件循环
        thread_->join();//等待线程退出
    }
}


EventLoop* EventLoopThread::startLoop() {
    //make_unique是C++14新标准，需要包含头文件memory
    thread_ = std::make_unique<std::thread>(std::bind(&EventLoopThread::threadFunc, this));
    EventLoop* loop = nullptr;
    {
        std::unique_lock<std::mutex> locker(mutex_);
        while(loop_ == nullptr) {
            cond_.wait(locker);
        }
        loop = loop_;
    }
    return loop;
}

void EventLoopThread::threadFunc() {
    EventLoop loop;
    if(callback_) {
        callback_(&loop);
    }

    {
        std::unique_lock<std::mutex> locker(mutex_);
        loop_ = &loop;
        //向主线程发送消息，通知线程已经创建好了
        cond_.notify_all();
    }

    loop.loop();
    std::unique_lock<std::mutex> locker(mutex_);
    loop_ = nullptr;
}









}












