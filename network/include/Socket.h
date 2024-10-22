#pragma once

struct tcp_info;

namespace network {
class Inet_address;

class Socket {
public:
 Socket(int sockfd) : sockfd_(sockfd) {}
 ~Socket();

 int fd() const {return sockfd_;}

 bool getTcpInfo(struct tcp_info* info) const;
 bool getTcpInfoString(char* buf, int len) const;

 void BindAddress(const Inet_address& localaddr);
 void Listen();


 int Accept(Inet_address* perradddr);

 void ShutdownWrite();

 void setTcpNoDelay(bool on);
 void setReuseAddr(bool on);
 void setReusePort(bool on);
 void setKeepAlive(bool on);
 
 private:
  int sockfd_;
}


}

