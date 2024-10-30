#include "network/Acceptor.h"

#include "Inet_address.h"
#include "EventLoop.h"
#include "SocketOps.h"
#include <glog/logging.h>

#include <unistd.h>
#include <error.h>
#include <fcntl.h>


using namespace network;

namespace network {

/**
 * @brief Construct a new Acceptor object
 */
Acceptor::Acceptor(EventLoop* Loop, 
                   const Inet_address& listen_addr, 
                   bool reuseport): loop_(Loop),
                                    listen_fd_(socketops::createNoblocking_stop(listen_addr.family_4())),
                                    listening_(false),
                                    active_cahnnel_(loop_, listen_fd_.fd()),
                                    idlefd_(::open("/dev/null", O_RDONLY | O_NONBLOCK)) {

    assert(idlefd_ >= 0);
    listen_fd_.setReuseAddr(true);
    listen_fd_.setReusePort(reuseport);
    listen_fd_.BindAddress(listen_addr);
    active_cahnnel_->setReadEventCallback(std::bind(&Acceptor::handle_read, this));
}

/**
 * @brief Destroy the Acceptor object
 */
Acceptor::~Acceptor() {
    ::close(idlefd_);
    active_cahnnel_->disableall();
    active_cahnnel_->remove();
}


void Acceptor::listen() {
    loop_->assertInLoopThread();
    listen_fd_.Listen();
    listening_ = true;
    active_cahnnel_->enableReading();/*开始处理客户端的连接请求*/
}

/**
 * @brief Handle the read event of the listen socket
 */
void Acceptor::handle_read() {
    //检查
    loop_->assertInLoopThread();
    /*处理客户端的连接请求*/
    Inet_address peer_addr;
    int connfd = listen_fd_.Accept(&peer_addr);

    if(connfd >= 0) {
        std::string info = peer_addr.Ip_Port_to_string();
        LOG(INFO) << "Accept new connection from " << info;
        if(new_connection_callback_) {
            //注意，不在这里处理连接
            new_connection_callback_(listen_fd_.fd(), peer_addr);
        } else {
            LOG(ERROR) << "No new connection callback";
            ::close(connfd);
        }
    } else {
        /*表示当前进程打开的文件描述符已达上限*/
        if(errno == EMFILE) {
            ::close(idlefd_);
            idlefd_ = ::accept(listen_fd_.fd(), NULL, NULL);
            ::close(idlefd_);
            idlefd_ = ::open("/dev/null", O_RDONLY | O_NONBLOCK);
        }
    }
}

/*关于坑位fd的处理*/
//int ret = accept( listenfd, (struct sockaddr*)&addr, sizeof(addr) );

//if (-1 == ret)
//{
  //if ( errno == EMFILE )
  //{
	 //关闭空闲文件描述符，释放 "坑"位
     //close(idlefd);
	 
	 //接受 clientfd
	 //clientfd = accept( listenfd, nullptr, nullptr);
	 //关闭 clientfd，防止一直触发 listenfd 上的可读事件
	 //close(clientfd);
	 
	 //重新占领 "坑"位
	 //idlefd = ::open("/dev/null", O_RDONLY | O_CLOEXEC);
  //}
//}
}







