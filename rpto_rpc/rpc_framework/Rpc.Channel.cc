#include "Rpc.Channel.h"
#include <cassert>

#include <glog/logging.h>
#include <google/protobuf/descriptor.h>

#include "rpc.pb.h"
using namespace network;

RpcChannel::RpcChannel() :
    codec_(std::bind(&RpcChannel::onRpcMessage, this, std::placeholders::_1, std::placeholders::_2)),
    services_(NULL) {
        LOG(INFO) << "RpcChannel created - " << this;
}

RpcChannel::RpcChannel(const TcpConnectionPtr& conn):
    codec_(std::bind(&RpcChannel::onRpcMessage, this, std::placeholders::_1, std::placeholders::_2)),
    services_(NULL),
    conn_(conn) {
        LOG(INFO) << "RpcChannel created - " << this;
}

RpcChannel::~RpcChannel() {
    LOG(INFO) << "RpcChannel deleted - " << this;
    for(const auto& outstanding : outstandings_) {
        OutStandingCall out = outstanding.second;
        delete out.responses;
        delete out.done;
    }
}

void RpcChannel::CallMethod(const golg_protobuf_MethodDescriptor* method,
                  golg_protobuf_RpcController* controller,
                  const golg_protobuf_Message* request,
                  golg_protobuf_Message* response,
                  golg_protobuf_Closure* done) {
    RpcMessage message;
    message.set_type(REQUEST);
    /*1) 进行原子前自增。等价于 fetch_add(1)+1 。

      2) 进行原子后自增。等价于 fetch_add(1) 。

      3) 进行原子前自减。等价于 fetch_sub(1)-1 。

      4) 进行原子后自减。等价于 fetch_sub(1) 。*/
    int64_t id = id_.fetch_add(1) + 1;
    message.set_id(id);
    //利用反射原理，获取请求的service和method的名称
    message.set_service(method->service()->full_name());
    message.set_method(method->name());
    message.set_request(request->SerializeAsString());

    OutStandingCall out = {response, done};
    {
        std::unique_lock<std::mutex> locker(mutex_);
        outstandings_[id] = out;
    }
    codec_.send(conn_, message);
}

void RpcChannel::onMessage(const TcpConnectionPtr& conn, Buffer* buf) {
    codec_.onMessage(conn, buf);
}

void RpcChannel::onRpcMessage(const TcpConnectionPtr& conn, const RpcMessagePtr& messageptr) {
    assert(conn == conn_);
    RpcMessage& message = *messageptr;
    if(message.type() == RESPONSE) {
        handle_response_msg(messageptr);
    } else if(message.type() == REQUEST) {
        handle_request_msg(messageptr);
    }
}