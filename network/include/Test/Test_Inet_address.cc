#include <gtest/gtest.h>
#include <netinet/in.h>
#include "/home/jianglei/protobuf_cs_muduo/network/include/Inet_address.h"

using namespace network;

TEST(InetAddressTest, DefaultConstructor) {
    Inet_address addr;
    EXPECT_EQ(addr.port(), 12345);
    EXPECT_EQ(addr.family_4(), AF_INET);
}

TEST(InetAddressTest, LoopbackConstructor) {
    Inet_address addr(12345, true);
    EXPECT_EQ(addr.Ip_to_string(), "127.0.0.1");
    EXPECT_EQ(addr.port(), 12345);
}

TEST(InetAddressTest, IPv6Constructor) {
    Inet_address addr(12345, false, true);
    EXPECT_EQ(addr.family_6(), AF_INET6);
}

TEST(InetAddressTest, IPPortConstructor) {
    Inet_address addr("192.168.1.1", 8080);
    EXPECT_EQ(addr.Ip_to_string(), "192.168.1.1");
    EXPECT_EQ(addr.port(), 8080);
}

TEST(InetAddressTest, InvalidIPConstructor) {
    Inet_address addr("invalid_ip", 8080);
    EXPECT_EQ(addr.Ip_to_string(), "0.0.0.0");
}

TEST(InetAddressTest, PortNetEndian) {
    Inet_address addr("192.168.1.1", 8080);
    EXPECT_EQ(addr.portNetEndian(), htons(8080));
}

TEST(InetAddressTest, IPv4NetEndian) {
    Inet_address addr("192.168.1.1", 8080);
    EXPECT_EQ(addr.ipv4NetEndian(), inet_addr("192.168.1.1"));
}

TEST(InetAddressTest, ResolveFunction) {
    Inet_address addr;
    EXPECT_TRUE(Inet_address::reslove("localhost", &addr));
    EXPECT_EQ(addr.Ip_to_string(), "127.0.0.1");
}

TEST(InetAddressTest, EdgeCaseZeroPort) {
    Inet_address addr("192.168.1.1", 0);
    EXPECT_EQ(addr.port(), 0);
}

TEST(InetAddressTest, EdgeCaseEmptyIP) {
    Inet_address addr("", 8080);
    EXPECT_EQ(addr.Ip_to_string(), "0.0.0.0");
}

// Run all tests
int main(int argc, char **argv) {
    ::testing::InitGoogleTest(&argc, argv);
    return RUN_ALL_TESTS();
    //::google::ShutdownGoogleLogging(); 
}

//在network目录下执行以下命令
//g++ -Iinclude include/Test/Test_Inet_address.cc src/Inet_address.cc src/SocketOps.cc -o Test_Inet_address -lgtest -lglog
