#pragma once 

#include <sys/types.h>
#include <unistd.h>

namespace network {
pid_t GetPid();

pid_t GetThreadId();

int64_t GetCurrentTime();

int32_t GetInt32FromNetByte(const char* buf);
}