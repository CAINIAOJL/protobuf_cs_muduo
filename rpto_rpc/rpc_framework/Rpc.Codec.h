#pragma once

#include "rpc.pb.h"

#include <string>
#include <memory>
#include <iostream>

#include <type_traits>

#define golgmessage ::google::protobuf::Message
#define gmessage google::protobuf::Message

namespace network {
class Buffer;
class TcpConnection;
typedef std::shared_ptr<TcpConnection> TcpConnectionPtr;

class RpcMessage;
typedef std::shared_ptr<RpcMessage> RpcMessagePtr;
extern const int rpctag[];

class ProtoRpcCodec {
 public:
  const static int KHeaderLen = sizeof(int32_t);
  const static int KChecksumLen = sizeof(int32_t);
  const static int KMaxMessageLen = 64 * 1024 * 1024;

  enum ErrorCode {
    KNoError = 0,           //not error
    KInvalidLength,         //invalid legnth
    KcheckSumError,        //check sum error
    KInvalidNameLen,        //invalid name length
    KUnKnownMessageType,    //un konwn message type
    KParseEroor,            //parse errpr
  };

  typedef std::shared_ptr<TcpConnection> TcpConnectionPtr;
  typedef std::function<void(const TcpConnectionPtr&, const RpcMessagePtr&)> RpcMessageCallback;

  typedef std::shared_ptr<gmessage> MessagePtr;

  explicit ProtoRpcCodec(const RpcMessageCallback& messagecb):messagecb_(messagecb) {}
  ~ProtoRpcCodec() {}
  void send(const TcpConnectionPtr& conn, const golgmessage& message);
  void onMessage(const tcpConnectionPtr& conn, Buffer* buffer);

  bool parseFromBuffer(const void* buf, int len, golgmessage* message);
  int serializeToBuffer(const golgmessage& message, Buffer* buf);

  ErrorCode parse(const char* buf, int len, golgmessage* message);

  void fillEmptyMessage(Buffer* buf, const golgmessage& message);   
  
  static int32_t checksum(const void* buf, int len);
  static bool vailddataChecksum(const char* buf, int len);
  static int32_t asInt32(const char* buf);

private:
  RpcMessageCallback messagecb_;
  int KMinMessageLen = 4;
  const std::string tag_ = "RPC0";//版本号

};
}
