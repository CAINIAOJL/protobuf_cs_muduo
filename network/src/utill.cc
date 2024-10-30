#include "network/utill.h"

#include <arpa/inet.h>
#include <string.h>
#include <sys/syscall.h>
#include <sys/types.h>
#include <unistd.h>
#include <sys/time.h>
//using namespace network;

static int g_pid = 0;
/*thread_local 关键字则确保每个线程都有一个独立的 t_pid 变量*/
static thread_local int t_pid = 0;

/**
 * @brief 获取当前进程的pid
 * @return pid
 */
pid_t network::GetPid() {
    if(g_pid != 0) {
        return g_pid;
    }
    return getpid();
}

/**
 * @brief 获取当前线程的tid
 * @return tid
 */
pid_t network::GetThreadId() {
    if(t_pid != 0) {
        return t_pid;
    }
    return static_cast<pid_t>(::syscall(SYS_gettid));
}

/**
 * @brief 获取当前时间
 * @param tv 时间结构体
 */
int64_t network::GetCurrentTime() {
    struct timeval tv;
    gettimeofday(&tv, NULL);
    return tv.tv_sec * 1000 + tv.tv_usec / 1000;
}

int32_t network::GetInt32FromNetByte(const char* buf) {
    int32_t value = 0;
    memcpy(&value, buf, sizeof(value));
    return ntohl(value);
}