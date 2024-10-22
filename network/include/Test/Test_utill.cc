/*TEST(utill_test, getpid_) {
    EXPECT_EQ(GetPid(), GetPid());
}

TEST(utill_test, gettid_) {
    EXPECT_EQ(GetThreadId(), GetThreadId());
}

TEST(utill_test, getcurrenttime_) {
    EXPECT_EQ(GetCurrentTime(), GetCurrentTime());
}

TEST(utill_test, gettimeofday_) {
    const char* buf = "";
    //memset(buf, 0, strlen(buf));
    EXPECT_EQ(GetInt32FromNetByte(buf), GetInt32FromNetByte(buf));
}

int main(int argc, char* argv[]) {
    testing::InitGoogleTest(&argc, argv);
    return RUN_ALL_TESTS();
}*/

/*namespace network {
    TEST(NetworkUtilTest, GetPid_HappyPath) {
        pid_t pid = GetPid();
        ASSERT_GT(pid, 0);
    }

    TEST(NetworkUtilTest, GetThreadId_HappyPath) {
        pid_t tid = GetThreadId();
        ASSERT_GT(tid, 0);
    }

    TEST(NetworkUtilTest, GetCurrentTime_HappyPath) {
        int64_t currentTime = GetCurrentTime();
        ASSERT_GT(currentTime, 0);
    }

    //使用unsigned char代替char：unsigned char的取值范围是0到255，可以安全地存储0xFF。
    TEST(NetworkUtilTest, GetInt32FromNetByte_ValidInput) {
        unsigned char buf[] = {0x00, 0x00, 0x01, 0x2c}; // 300 in network byte order
        int32_t result = GetInt32FromNetByte(buf);
        ASSERT_EQ(result, 300);
    }

    TEST(NetworkUtilTest, GetInt32FromNetByte_InvalidInput) {
        unsigned char buf[] = {0xFF, 0xFF, 0xFF, 0xFF}; // -1 in network byte order
        int32_t result = GetInt32FromNetByte(buf);
        ASSERT_EQ(result, -1);
    }

    TEST(NetworkUtilTest, GetInt32FromNetByte_EmptyInput) {
        char buf[] = {};
        ASSERT_DEATH(GetInt32FromNetByte(buf), ""); // Assuming it might crash on empty input
    }

    TEST(NetworkUtilTest, GetInt32FromNetByte_NonFourByteInput) {
        unsigned char buf[] = {0x00, 0x00, 0x01}; // Not enough bytes for a valid int32
        ASSERT_DEATH(GetInt32FromNetByte(buf), ""); // Assuming it might crash
    }
}*/

#include <unistd.h>
#include <cstring>
#include <gtest/gtest.h>
#include <sys/syscall.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include "/home/jianglei/protobuf_cs_muduo/network/include/utill.h"
using namespace network;

//extern "C" {
    //pid_t GetPid();
    //pid_t GetThreadId();
    //int64_t GetCurrentTime();
    //int32_t GetInt32FromNetByte(const char* buf);
//}

TEST(UtilTest, GetPidTest) {
    pid_t pid = GetPid();
    EXPECT_EQ(pid, ::getpid());
}

TEST(UtilTest, GetThreadIdTest) {
    pid_t tid = GetThreadId();
    EXPECT_EQ(tid, static_cast<pid_t>(::syscall(SYS_gettid)));
}

TEST(UtilTest, GetCurrentTimeTest) {
    int64_t time1 = GetCurrentTime();
    sleep(1);
    int64_t time2 = GetCurrentTime();
    EXPECT_GE(time2, time1 + 1000); // Should be at least 1 second later
}

TEST(UtilTest, GetInt32FromNetByteTest) {
    int32_t original = 123456;
    char buffer[4];
    memcpy(buffer, &original, sizeof(original));
    int32_t converted = GetInt32FromNetByte(buffer);
    EXPECT_EQ(converted, ::ntohl(original));
}

TEST(UtilTest, GetInt32FromNetByteEdgeCases) {
    // Test with minimum value
    int32_t minValue = INT32_MIN;
    char minBuffer[4];
    memcpy(minBuffer, &minValue, sizeof(minValue));
    EXPECT_EQ(GetInt32FromNetByte(minBuffer), ::ntohl(minValue));

    // Test with maximum value
    int32_t maxValue = INT32_MAX;
    char maxBuffer[4];
    memcpy(maxBuffer, &maxValue, sizeof(maxValue));
    EXPECT_EQ(GetInt32FromNetByte(maxBuffer), ::ntohl(maxValue));

    // Test with zero
    int32_t zeroValue = 0;
    char zeroBuffer[4];
    memcpy(zeroBuffer, &zeroValue, sizeof(zeroValue));
    EXPECT_EQ(GetInt32FromNetByte(zeroBuffer), ::ntohl(zeroValue));
}

int main(int argc, char **argv) {
    ::testing::InitGoogleTest(&argc, argv);
    return RUN_ALL_TESTS();
}

//在network目录下
//执行g++ -Iinclude include/Test/Test_utill.cc src/utill.cc -o Test_utill -lgtest -lglog