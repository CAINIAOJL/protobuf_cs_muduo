#include "network/Poller.h"
#include "network/Channel.h"
#include <glog/logging.h>

#include <poll.h>
#include <sys/epoll.h>
#include <errno.h>
#include <assert.h>


using namespace network;

static_assert(EPOLLIN == POLLIN, "EPOLLIN!= POLLIN");
static_assert(EPOLLOUT == POLLOUT, "EPOLLOUT!= POLLOUT");
static_assert(EPOLLERR == POLLERR, "EPOLLERR!= POLLERR");
static_assert(EPOLLHUP == POLLHUP, "EPOLLHUP!= POLLHUP");
static_assert(EPOLLPRI == POLLPRI, "EPOLLPRI!= POLLPRI");
static_assert(EPOLLRDHUP == POLLRDHUP, "EPOLLRDHUP!= POLLRDHUP");

namespace Kstate {
    const int KNew = -1;
    const int KAdded = 1;
    const int KDeleted = 2;
}


namespace network {
/**
 * @brief Construct a new Poller object
 */
Poller::Poller(EventLoop* Loop):ownerLoop_(Loop),
                                epollfd_(::epoll_create1(EPOLL_CLOEXEC)),
                                events_(KInitEventListSize){
    if(epollfd_ < 0) {
        LOG(FATAL) << "create epoll failed";
    }
}

/**
 * @brief Destroy the Poller object
 */
Poller::~Poller() {
    /*析构函数，关闭epoll句柄*/
    ::close(epollfd_);
}

/**
 * @brief Poller的核心函数之一，处理epoll_wait的返回值，并将活跃的channel放入activeChannels中
 * @param timeouts 超时时间，单位为毫秒
 * @param activeChannels 活跃的channel列表
 */
void Poller::poll(int timeouts, ChannelList* activeChannels) {
    LOG(INFO) << "Poller::poll()" << " timeout=" << timeouts; << "total fd count = " << channels_.size();
    int numEvents = ::epoll_wait(epollfd_, &*events_.begin(), static_cast<int>(events_.size(), timeouts));

    int savedErrno = errno;
    if(numEvents > 0) {
        LOG(INFO) << "Poller::poll() got events: " << numEvents;
        /*实际处理的函数*/
        fillActiveChannels(numEvents, activeChannels);
        if(size_t(numEvents) == events_.size()) {
            events_.resize(events_.size() * 2);/*扩容*/
        }
    } else if(numEvents == 0) {
        LOG(INFO) << "Poller::poll() nothing happened";
    } else if(numEvents < 0) {
        if(savedErrno != EINTR) {
            errno = savedErrno;
            LOG(ERROR) << "Poller::poll() epoll_wait error";
        }
    }
}

/**
 * @brief 向activeChannels中添加活跃的channel
 * @param numEvents 事件数量
 * @param activeChannels 活跃的channel列表
 */
void Poller::fillActiveChannels(int numEvents, ChannelList* activeChannels) {
    assert(size_t(numEvents) <= events_.size());/*检查*/
    for(int i = 0; i < numEvents; i++) {
        /*重点，event事件中，保存了一个指针，指向channel对象*/
        /*用这个执政指向Channel对象*/
        Channel* channel = static_cast<Channel*>(events_[i].data.ptr);
/*没有DEBUG宏，则下面这段代码会被优化掉*/
#ifndef DEBUG
        int fd = channel->fd();
        ChannelMap::iterator it = channels_.find(fd);
        assert(it != channels_.end());
        assert(it->second == channel);
#endif
        channel->setRevent(events_[i].events);
        /*设置了event的channel，就是活跃的channel*/
        activeChannels->push_back(channel);
    }
}

/**
 * @brief 向epoll中添加或更新channel
 * @param channel 要添加或更新的channel
 */
void Poller::updateChannel(Channel* channel) {
    Poller::assertInLoopThread();
    const int index = channel->index();
    LOG(INFO) << "fd = " << channel->fd() << " events = " << channel->event() << " index = " << channel->index();
    if(index == Kstate::KNew || index == Kstate::KDeleted) {
        /*如果是新的channel， 或者是删除了的channel，需要重新加入到epoll*/
        /*因为channel中保存了fd，所以这里直接用fd来判断*/
        int fd = channel->fd();
        if(index == Kstate::KNew) {
            assert(channels_.find(fd) == channels_.end());
            /*添加到channels_中*/
            channels_[fd] = channel;
        } else {
            /*index == Kstate::KDeleted*/
            /*意味着channel已经从epoll中删除了, 但是channels_中还保留着这个channel*/
            assert(channels_.find(fd) != channels_.end());
            assert(channels_[fd] == channel);
        }

        channel->setIndex(Kstate::KAdded);
        update(EPOLL_CTL_ADD, channel);
    } else {
        /*如果是已经在epoll中的channel，则更新它的event*/
        assert(index == Kstate::KAdded);
        int fd = channel->fd();
        assert(channels_.find(fd) != channels_.end());
        assert(channels_[fd] == channel);
        //update(EPOLL_CTL_MOD, channel);
        if(channel->isNoneEvent()) {
            update(EPOLL_CTL_DEL, channel);
            channel->setIndex(Kstate::KDeleted);
        } else {
            update(EPOLL_CTL_MOD, channel);
        }
    }
}

/**
 * @brief 将channel中的fd从epoll中删除
 */
void Poller::removeChannel(Channel* channel) {
    Poller::assertInLoopThread();
    int fd = channel->fd();
    assert(channels_.find(fd) != channels_.end());
    assert(channels_[fd] == channel);
    /*检查channel中是否还有事件*/
    assert(channel->isNoneEvent());
    int index = channel->index();
    assert(index == Kstate::KAdded || index == Kstate::KDeleted);
    size_t n = channels_.erase(fd);
    (void)n;
    assert(n == 1);

    if(index == Kstate::KAdded) {
        update(EPOLL_CTL_DEL, channel);
    }
    /*设置为新的状态*/
    channel->setIndex(Kstate::KNew);
}

/**
 * @brief 更新的核心函数，向epoll中添加或更新或删除channel
 */
void Poller::update(int operation, Channel* channel) {
    struct epoll_event event;
    memset(&event, 0, sizeof(event));
    event.events = channel->event();
    event.data.ptr = channel;
    int fd = channel->fd();

    /*调用epoll_ctl*/
    if(::epoll_ctl(epollfd_, operation, fd, &event) < 0) {
        /*未使用operationToString(int op)*/
        if(operation == EPOLL_CTL_DEL) {
            LOG(INFO) << "Poller::update() epoll_ctl on "<< operationToString(operation) << " fd = " << fd;
        } else {
            LOG(FATAL) << "Poller::update() epoll_ctl on "<< operationToString(operation) << " fd = " << fd;
        }
    }
}

const char* Poller::operationToString(int op) {
    switch (op)
    {
    case EPOLL_CTL_ADD:
        return "EPOLL_CTL_ADD";
    case EPOLL_CTL_MOD:
        return "EPOLL_CTL_MOD";
    case EPOLL_CTL_DEL:
        return "EPOLL_CTL_DEL";
    default:
        return "Unknown Operation";
    }
}

}



