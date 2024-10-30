#pragma once 

#include <vector>
#include <unistd.h>
#include <cassert>
#include <string.h>
#include <algorithm>
#include <errno.h>
#include <string>

#include "Endian.h"

/*muduo库中对于缓冲区的设计, 设计思路来自unix网络编程卷一（套接字联网api）*/

/// A buffer class modeled after org.jboss.netty.buffer.ChannelBuffer
///
/// @code
/// +-------------------+------------------+------------------+
/// | prependable bytes |  readable bytes  |  writable bytes  |
/// |                   |     (CONTENT)    |                  |
/// +-------------------+------------------+------------------+
/// |                   |                  |                  |
/// 0      <=      readerIndex   <=   writerIndex    <=     size


namespace network {
class Buffer {

public:
 static const size_t KCheapPrependSize;//预留空间大小
 static const size_t KinitialBufferSize;//初始缓冲区大小
 explicit Buffer(size_t initalsize = KinitialBufferSize);

 //返回可读字节数 
 size_t readableBytes() const {return writeIndex_ - readerIndex_;}
 //返回可写字节数
 size_t writeableBytes() const {return buffer_.size() - writeIndex_;}
 //返回可预留字节数
 size_t prependableBytes() const {return readerIndex_;}
 
 //
 size_t Capacity() const {return buffer_.capacity();}

 size_t Size() const {return buffer_.size();}

 const char* peek() const {return begin() + readerIndex_;}
 
 /**
  * @brief retrieve = 取回, 取回len个字节的数据 说白了就叫读取函数,不返回数据，只改动readerIndex_, writeIndex_指针
  */
 void retrieve(size_t len) {
    assert(len <= readableBytes());
    if(len < readableBytes()) {
        //取回数据，需要将readerIndex_向前移动
        readerIndex_ += len;
    } else {
        retrieveAll();
    }
 }
 
 void retrieveInt64() {retrieve(sizeof(int64_t));}
 void retrieveInt32() {retrieve(sizeof(int32_t));}
 void retrieveInt16() {retrieve(sizeof(int16_t));}
 void retrieveInt8() {retrieve(sizeof(int8_t));}

 /**
  * @brief 取回所有可读字节
  */
 void retrieveAll() {
    readerIndex_ = KCheapPrependSize;
    writeIndex_ = KCheapPrependSize;
 }
 
 //获取可读字节数的字符串
 std::string retrieveAllAsString() {
    return retrieveAllAsString(readableBytes());
 }
 
 //获取指定长度的字符串
 std::string retrieveAllAsString(size_t len) {
    assert(len <= readableBytes());
    std::string result(peek(), len);
    retrieve(len);
    return result;
 }

 char* beginWrite() {return begin() + writeIndex_;}
 const char* beginWrite() const {return begin() + writeIndex_;}

 void hasWritten(size_t len) {
    assert(len <= writeableBytes());
    writeIndex_ += len;
 }

 void unWrite(size_t len) {
    assert(len <= readableBytes());
    writeIndex_ -= len;
 }

 //确保可写入
 void ensureWriteableBytes(size_t len) {
    if(writeableBytes() < len) {
        makespace(len);
    }
    //检查腾出空间后是否还可以写入
    assert(writeableBytes() >= len);
 }
  
 void append(const std::string& str) {append(str.data(), str.size());}
 void append(const void* data, size_t len) {append(static_cast<const char*>(data), len);}

 //在将数据加入到缓冲区后，要及时更新writerIndex_, 否则会导致数据丢失
 void append(const char* data, size_t len) {
    ensureWriteableBytes(len);
    std::copy(data, data + len, beginWrite());
    hasWritten(len);
 }
 
 /////////////////////////////////////////////////////
 /*用于rpc协议中，将int64_t, int32_t, int16_t, int8_t等类型序列化到缓冲区中*/
 void appendInt64(int64_t x) {
    int64_t be64 = endian::host_to_net_64(x);
    append(&be64, sizeof(be64));
 }
 void appendInt32(int32_t x) {
    int32_t be32 = endian::host_to_net_32(x);
    append(&be32, sizeof(be32));
 }
 void appendInt16(int16_t x) {
    int16_t be16 = endian::host_to_net_16(x);
    append(&be16, sizeof(be16));
 }
 void appendInt8(int8_t x) {
    append(&x, sizeof(x));
 }
 
 int64_t readInt64() {
    int64_t result = peekInt64();
    retrieveInt64();
    return result;
 }
 int32_t readInt32() {
    int32_t result = peekInt32();
    retrieveInt32();
    return result;
 }
 int16_t readInt16() {
    int16_t result = peekInt16();
    retrieveInt16();
    return result;
 }
 int8_t readInt8() {
    int8_t result = peekInt8();
    retrieveInt8();
    return result;
 }

 int64_t peekInt64() const {
    assert(readableBytes() >= sizeof(int64_t));
    int64_t be64 = 0;
    ::memcpy(&be64, peek(), sizeof(be64));
    return endian::net_to_host_64(be64);
 }
 int32_t peekInt32() const {
    assert(readableBytes() >= sizeof(int32_t));
    int32_t be32 = 0;
    ::memcpy(&be32, peek(), sizeof(be32));
    return endian::net_to_host_32(be32);
 }
 int16_t peekInt16() const {
    assert(readableBytes() >= sizeof(int16_t));
    int16_t be16 = 0;
    ::memcpy(&be16, peek(), sizeof(be16));
    return endian::net_to_host_16(be16);
 }
 int8_t peekInt8() const {
    assert(readableBytes() >= sizeof(int8_t));
    int8_t x = *peek();
    return x;
 }
 
 void prepend(int64_t x) {
    int64_t be64 = endian::host_to_net_64(x);
    prepend(&be64, sizeof(be64));
 }
 void prepend(int32_t x) {
    int32_t be32 = endian::host_to_net_32(x);
    prepend(&be32, sizeof(be32));
 }
 void prepend(int16_t x) {
    int16_t be16 = endian::host_to_net_16(x);
    prepend(&be16, sizeof(be16));
 }
 void prepend(int8_t x) {
    prepend(&x, sizeof(x));
 }
 
 void prepend(const void* data, size_t len) {
    assert(len <= prependableBytes());
    readerIndex_ -= len;
    const char* d = static_cast<const char*>(data);
    std::copy(d, d + len, begin() + readerIndex_);
 }
 //////////////////////////////////////////////////

 ssize_t readFd(int fd, int* saveerrno);
private:
 char* begin() {return &*buffer_.begin();}
 const char* begin() const {return &*buffer_.begin();}

 //空间转移，避免vector扩容
 void makespace(size_t len) {
    if(writeableBytes() + prependableBytes() < len + KCheapPrependSize) {
        buffer_.resize(writeIndex_ + len);
    } else {
        assert(KCheapPrependSize < readerIndex_);
        size_t readable = readableBytes();
        std::copy(begin() + readerIndex_, begin() + writeIndex_, begin() + KCheapPrependSize);
        readerIndex_ = KCheapPrependSize;
        writeIndex_ = readerIndex_ + readable;
        assert(readable == readableBytes());
    }
 }

  std::vector<char> buffer_;
  size_t readerIndex_;
  size_t writeIndex_;
  /*通常，Windows 使用 CRLF 作为换行符，而 Unix/Linux 和 macOS 只使用 LF。*/
  static const char KCRLF[];
};
}

