#pragma once

#include <mutex>
#include <map>

#include <google/protobuf/service.h>

#include "Rpc.Codec.h"
#include "rpc.pb.h"

#define golg_protobuf_service ::google::protobuf::Service
#define golg_protobuf_MethodDescriptor ::google::protobuf::MethodDescriptor
#define golg_protobuf_RpcController ::google::protobuf::RpcController
#define golg_protobuf_Message ::google::protobuf::Message
#define golg_protobuf_Closure ::google::protobuf::Closure




namespace google {
namespace protobuf {

class Descriptor;         // descriptor.h
class ServiceDescriptor;  // descriptor.h
class MethodDescriptor;   // descriptor.h
class Message;            // message.h

class Closure;  // closure.h

class RpcController;  // controller.h
class Service;        // service.h
}
}

//stub Service 存根服务器 ---》rpc框架的核心

namespace network {
class RpcChannel:public google::protobuf::RpcChannel {
 public:
  RpcChannel();
  explicit RpcChannel(const TcpConnectionPtr& conn);
  ~RpcChannel() override;

  void setConnection(const TcpConnectionPtr& conn) {conn_ = conn;}
  void setService(const std::map<std::string, golg_protobuf_service*>* services) {
    services_ = services;
  }
  
  void CallMethod(const golg_protobuf_MethodDescriptor* method,
                  golg_protobuf_RpcController* controller,
                  const golg_protobuf_Message* request,
                  golg_protobuf_Message* response,
                  golg_protobuf_Closure* done) override;
  void onMessage(const TcpConnectionPtr& conn, Buffer* buf);

private:
  void onRpcMessage(const TcpConnectionPtr& conn, const RpcMessagePtr& messageptr);
  void doneCallback(golg_protobuf_Message* response, int64_t id);

  void handle_response_msg(const RpcMessagePtr& messageptr);
  void handle_request_msg(const TcpConnectionPtr& conn, const RpcMessagePtr& messageptr);

  struct OutStandingCall {
    golg_protobuf_Message* responses;
    golg_protobuf_Closure* done;
  };

  ProtoRpcCodec codec_;
  std::mutex mutex_;
  std::atomic<int64_t> id_;//原子变量，保证线程安全

  std::map<int64_t, OutStandingCall>  outstandings_;

  TcpConnectionPtr conn_;
  const std::map<std::string, golg_protobuf_service*>* services_;
};

typedef std::shared_ptr<RpcChannel> RpcChannelPtr;
}