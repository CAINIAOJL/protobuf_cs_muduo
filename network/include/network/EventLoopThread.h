#pragma once
//主要运用了c++11中的线程、条件变量、互斥锁等技术
//不同于Linux下的自行创建mutex与condition变量，c++11中的mutex与condition变量是线程安全的，可以直接使用
//c++11 中的线程类 
#include <thread>
//c++11 中的条件变量类
#include <condition_variable>
#include <cassert>
//c++11 中的互斥锁类
#include <mutex>
#include <functional>
#include <memory>

//EventLoopthread ----EVentLoop线程，是线程池的单位
namespace network {
class EventLoop;
class EventLoopThread {
public:
 typedef std::function<void(EventLoop*)> ThreadInitCallback; //线程初始化回调函数类型
 
 EventLoopThread(const ThreadInitCallback& cb = ThreadInitCallback(),
                 const std::string& name = std::string()); //构造函数

 ~EventLoopThread(); //析构函数

  EventLoop* startLoop(); //启动线程，返回事件循环对象

private:
 void threadFunc(); //线程函数

 EventLoop* loop_; //事件循环
 bool isStarting_; //是否正在启动
 std::unique_ptr<std::thread> thread_; //线程对象
 std::mutex mutex_; //互斥锁
 std::condition_variable cond_; //条件变量
 ThreadInitCallback callback_; //线程初始化回调函数

};
}