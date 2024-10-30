#pragma once

#include <functional>
#include <memory>
#include "Buffer.h"


namespace network {

//占位符，用于回调函数参数的占位符, std::bind()
using std::placeholders::_1;
using std::placeholders::_2;
using std::placeholders::_3;

template <typename T>
inline T* get_pointer(const std::shared_ptr<T>& p) {
    return p.get();
}

template <typename T>
inline T* get_pointer(const std::unique_ptr<T>& p) {
    return p.get();
}

class Buffer;
class TcpConnection;
//共享指针
typedef std::shared_ptr<TcpConnection> TcpConnectionptr;
typedef std::function<void()> TimerCallback;
//参数为TcpConnectionptr的回调函数
typedef std::function<void(const TcpConnectionptr&)> ConnectionCallback;
typedef std::function<void(const TcpConnectionptr&)> CloseCallback;

//低水位回调函数
typedef std::function<void(const TcpConnectionptr&)> WriteCompleteCallback;
//高水位回调函数
typedef std::function<void(const TcpConnectionptr&, size_t)> HighWaterMarkCallback;

typedef std::function<void(const TcpConnectionptr&, Buffer*)> MessageCallback;

void defaultConnectionCallback(const TcpConnectionptr& conn);
void defaultMessageCallback(const TcpConnectionptr&, Buffer*);

}

