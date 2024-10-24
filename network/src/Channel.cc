#include "EventLoop.h"
#include "Channel.h"
#include <poll.h>
#include <cassert>
using namespace network; 

namespace network {
/**
 * @brief Construct a new Channel object
 */
explicit Channel::Channel(EventLoop* loop, int fd):
                loop_(loop), 
                fd_(fd),
                events_(0),
                revents_(0),
                index_(-1),
                eventHandling_(false),
                addedToLoop_(false)
                {}

Channel::~Channel() {
    /*检查*/
    assert(eventHandling_ == false);
    assert(addedToLoop_ == false);

    if(loop_->isInLoopThread()) {
        loop_->removeChannel(this);
    }
}

void Channel::update() {
    addedToLoop_ = true;
    loop_->updateChannel(this);/*this = *Channel*/
}

void Channel::remove() {
    assert(isNoneEvent());
    addedToLoop_ = false;
    loop_->removeChannel(this);
}

/*poller的模型*/
void Channel::handleEvent() {
    eventHandling_ = true;
    /*用户自定义的事件处理函数*/
    if((revents_ & POLLHUP) && !(revents_ & POLLIN)) {
        if(closeCallback_) {
            closeCallback_();
        }
    }

    //读
    if(revents_ &(POLLIN | POLLPRI | POLLRDHUP)) {
        if(readCallback_) {
            readCallback_();
        }
    }

    //错误
    if(revents_ &(POLLERR | POLLNVAL)) {
        if(errorCallback_) {
            errorCallback_();
        }
    }

    if(revents_ & POLLOUT) {
        if(writeCallback_) {
            writeCallback_();
        }
    }
    eventHandling_ = false;
}
}