#pragma once

#include "EventLoop.h"
#include <map>
#include <vector>

struct epoll_event;

namespace network {
class Channel;

class Poller {
public:
 typedef std::vector<Channel*> ChannelList;
 Poller(EventLoop* Loop);
 ~Poller();

 void poll(int timeouts, ChannelList* activeChannels);
 void updateChannel(Channel* channel);
 void removeChannel(Channel* channel);


 void assertInLoopThread() {ownerLoop_->assertInLoopThread();}
 bool hasChannel(Channel* channel) const;

 private:
 EventLoop* ownerLoop_;
 static const int KInitEventListSize = 16;

 static const char* operationToString(int op);
 void fillActiveChannels(int numEvents, ChannelList* activeChannels);
 void update(int operation, Channel* channel);
 
 typedef std::vector<struct epoll_event> EventList;
 int epollfd_;
 /*存储感兴趣的事件*/
 EventList events_;

 /*poller中有许多Channel*/
 /*ChannelMap的key是fd，value是Channel*/
 typedef std::map<int, Channel*> ChannelMap;
 ChannelMap channels_; 

};
}