#include "network/EventLoop.h"
#include "network/Poller.h"
#include <glog/logging.h>
//#include <Poller.h>
#include <Channel.h>



#include <signal.h>
#include <sys/eventfd.h>
#include <unistd.h>

#include <algorithm>
#include <cassert>

using namespace network;

namespace network {
    /*thread_local关键字声明了一个线程局部变量，它只在当前线程中有效，并且在线程结束时自动销毁。*/
    static thread_local EventLoop* t_loopInThisThread = nullptr;

    const int PollerTimes = 10000;

    /*eventfd 是一个计数相关的fd。计数不为零是有可读事件发生，read 之后计数会清零，write 则会递增计数器。*/
    int createEventfd() {
        int eventfd = ::eventfd(0, EFD_NONBLOCK | EFD_CLOEXEC);
        if(eventfd < 0) {
            LOG(ERROR) << "Failed in createEventfd function";
            /* Abort execution and generate a core-dump.  */
            abort();
        }
        return eventfd;
    }
}

/**
 * @brief 获取当前线程的EventLoop
 * @return 当前线程的EventLoop
 */
EventLoop* EventLoop::getEventLoopofCurrentThread() {
    return t_loopInThisThread;
}

/**
 * @brief 构造函数
 */
EventLoop::EventLoop():looping_(false),
                        quit_(false),
                        eventHanding_(false),
                        iteration_(0),
                        poller_(new Poller(this)),
                        wakeupfd_(createEventfd()),
                        wakeupChannel_(new Channel(this, wakeupfd_)),
                        currentActiveChannel_(NULL) {
    LOG(INFO) << "EventLoop created " << this << "in thread" << threadPid;
    if(t_loopInThisThread) {
        LOG(FATAL) << "Another EventLoop " << t_loopInThisThread << "exists in this thread" << threadPid;
    } else {
        t_loopInThisThread = this;
    }

    wakeupChannel_->setReadEventCallback(std::bind(&EventLoop::handleRead, this));
    wakeupChannel_->enableReading();

    threadPid = GetThreadId();
}

/*思想是one thread one loop*/
/**
 * @brief 析构函数  
 */
EventLoop::~EventLoop() {
    wakeupChannel_->disableall();
    wakeupChannel_->remove();
    ::close(wakeupfd_);
    t_loopInThisThread = nullptr;/*清除线程局部变量*/
}

/**
 * @brief 启动事件循环
 *
 */
EventLoop::loop() {
    assert(!looping_);
    assertInLoopThread();
    looping_ = true;
    quit_ = false;
    LOG(INFO) << "EventLoop " <<  this << "start looping";

    /*循环开始的时候*/
    while(!quit_) {
        activeChannels_.clear();
        //后期改成 epoll
        //Poller_->poll(PollerTimes, activeChannels_);
        ++iteration_;
        eventHanding_ = true;
        for(auto it = activeChannels_.begin(); it != activeChannels_.end(); it++) {
            currentActiveChannel_ = *it;
            currentActiveChannel_->handleEvent();
        }
        currentActiveChannel_ = NULL;
        eventHanding_ = false;
        //处理任务
        dopendingFunctors();
    }
    LOG(INFO) << "EventLoop" << this << "stop looping";
    looping_ = false;
}

/**
 * @brief 退出事件循环
 */
void EventLoop::quit() {
    quit_ = true;
    if(!isInloopthread()) {
        wakeup();//唤醒阻塞的线程
    }
}

/**
 * @brief  如果在loop线程中，则直接执行cb，否则放到队列中
 */
void EventLoop::runInLoop(Functor cb) {
    //在这个线程中执行
    if(isInloopthread()) {
        cb();
    } else {
        //放到队列中
        queueInLoop(std::move(cb));
    }
}

/**
 * @brief  放到队列中
 */
void EventLoop::queueInLoop(Functor cb) {
    {
        std::unique_lock<std::mutex> locker(mutex_);
        pendingFunctors_.push_back(std::move(cb));
    }
    if(!isInloopthread() || callingPendingFunctors_) {
        wakeup();/*唤醒阻塞的线程*/
    }
}

/**
 * @brief 返回队列中待执行的任务数量
 */
size_t EventLoop::queueSize() const {
    std::unique_lock<std::mutex> locker(mutex_);
    return pendingFunctors_.size();
}

/**
 * @brief 更新channel，在Poller中实现
 */
void EventLoop::updateChannel(Channel* channel) {
    assert(channel->ownerLoop() == this);
    assertInLoopThread();
    //Poller_->updateChannel(channel);
}

/**
 * @brief 移除channel，在Poller中实现
 */
void EventLoop::removeChannel(Channel* channel) {
    assert(channel->ownerLoop() == this);
    assertInLoopThread();
    if(eventHanding_) {
        assert(currentActiveChannel_ == channel ||
               std::find(activeChannels_.begin(), activeChannels_.end(), channel) == activeChannels_.end())
    }
    //Poller_->removeChannel(channel);
}

/**
 * @brief channel是否存在
 */
bool EventLoop::hasChannel(Channel* channel) {
    asser(channel->ownerLoop() == this);
    assertInLoopThread();
    //return Poller_->hasChannel(channel);
}

void EventLoop::AortNotInLoopThread() {
    LOG(FATAL) << "Event loop is not in the current thread, threadID: "
              << threadPid << ", current threadID = " << GetThreadId();
}

/**
 * @brief 唤醒阻塞的线程
 */
void EventLoop::wakeup() {
    uint64_t one = 1;
    ssize_t ret = ::write(wakeupfd_, &one, sizeof(one));
    if(ret != sizeof(one)) {
        LOG(ERROR) << "EventLoop::wakeup() writes " << ret << " bytes instead of " << sizeof(uint64_t);
    }
}

/**
 * @brief 对应wakeup
 */
void EventLoop::handleRead() {
    /*对应wakeup*/
    /*接受wakeup传输的一个字节*/
    /*巧妙在eventfd，有一个字节的写入读取，就能唤醒阻塞的线程*/
    uint64_t one = 1;
    ssize_t ret = ::read(wakeupfd_, &one, sizeof(one));
    if(ret != sizeof(one)) {
        LOG(ERROR) << "EventLoop::handleRead() reads " << ret << " bytes instead of " << sizeof(uint64_t);
    }
}

/**
 * 在其他的线程中，任务无法执行，先放在队列中，等到回到正确的线程中执行
 * @brief 执行队列中的任务
 */
void EventLoop::dopendingFunctors() {
    std::vector<Functor> functors;
    callingPendingFunctors_ = true;
    //加锁
    {
        std::unique_lock<std::mutex> locker(mutex_);
        std::swap(functors, pendingFunctors_);
    }
    for(functors& cb : functors) {
        cb();
    }
    callingPendingFunctors_ = false;
}

void EventLoop::debugString() {}