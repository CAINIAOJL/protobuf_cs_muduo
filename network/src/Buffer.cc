#include "network/Buffer.h"
#include <sys/uio.h>
using namespace network;

namespace network {
 const size_t KCheapPrependSize = 8;//预留空间大小，默认8字节
 const size_t KinitialBufferSize = 1024 * 4;//初始缓冲区大小，默认4KB
/**
 * @brief 从文件描述符读取数据到缓冲区,分散读，聚集写
 * @brief 功能：将数据从文件描述符读到分散的内存块中，即分散读
 * @param fd 文件描述符
 * @param saveerrno 保存错误信息
 * @return 读取的字节数
 */
ssize_t Buffer::readFd(int fd, int* saveerrno) {
    /*@brief readv(), writev()函数的作用是将多个缓冲区的内容写入到一个文件描述符，或者从一个文件描述符读取多个缓冲区的内容。
    这两个函数的作用类似于read()和write()函数，但是它们可以一次操作多个缓冲区。*/
    char extrabuf[1024 * 1024];
    struct iovec iov[2];
    iov[0].iov_base = begin() + writeIndex_;
    iov[0].iov_len = writeableBytes();
    iov[1].iov_base = extrabuf;
    iov[1].iov_len = sizeof(extrabuf);
    const size_t writeable = writeableBytes();
    const int iovcnt = (writeable  < sizeof(extrabuf) ? 2 : 1);
    const ssize_t n = ::readv(fd, iov, iovcnt);
    if(n < 0) {
        *saveerrno = errno;
    } else if(size_t(n) <= writeable) {
        //读到的数据数小于我的缓冲区
        writeIndex_ += n;
    } else {
        //大于我的缓冲区
        /*现在我的缓冲区已经满了，*/
        //写指针更新到缓冲区的末尾
        writeIndex_ = buffer_.size();
        //我的缓冲区的剩余区域已经被填满，vector会自动扩容，将extrabuf中的数据加入到扩容后的vector中，
        //需要加入的数据量为读取到的数据量减去之前已经被写入到buffer中的数据量（也就是之前的可以写入的值writeable）
        append(extrabuf, n - writeable);
    }
    return n;
}

}