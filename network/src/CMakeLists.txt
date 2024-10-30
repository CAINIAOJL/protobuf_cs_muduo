include(CheckFunctionExists)

check_function_exists(accept4 HAVE_ACCEPT4)
if(NOT HAVE_ACCEPT4)
  set_source_files_properties(SocketsOps.cc PROPERTIES COMPILE_FLAGS "-DNO_ACCEPT4")
endif()

set(net_SRCS
  Accepto.cc
  Buffer.cc
  Channel.cc
  connector.cc
  EventLoop.cc
  EventLoopThread.cc
  EventLoopThreadPool.cc
  Inet_address.cc
  Poller.cc
  Socket.cc
  SocketOps.cc
  TcpClient.cc
  TcpConnection.cc
  TcpServer.cc
  utill.cc
  )

add_library(network ${net_SRCS})
target_link_libraries(network  glog)
target_include_directories(network PUBLIC
  ../include
  )


install(TARGETS  network DESTINATION  ${PROJECT_BINARY_DIR}/lib)