#pragma once

//endian 是关于字节序的头文件，它定义了一些宏，用于在不同字节序的机器上进行字节序转换。
#include <endian.h>
#include <stdint.h>

namespace network {
namespace endian {
    //从host字节序到网络字节序的转换
    inline uint16_t host_to_net_16(uint16_t host_16) { return htobe16(host_16);}
    inline uint32_t host_to_net_32(uint32_t host_32) { return htobe32(host_32);}
    inline int64_t host_to_net_64(uint64_t host_64) { return htole64(host_64);}

    //从网络字节序到host字节序的转换
    inline uint16_t net_to_host_16(uint16_t net_16) { return be16toh(net_16);}
    inline uint32_t net_to_host_32(uint32_t net_32) { return be32toh(net_32);}
    inline int64_t net_to_host_64(uint64_t net_64) { return le64toh(net_64);}
#pragma GCC diagnostic pop
}
}