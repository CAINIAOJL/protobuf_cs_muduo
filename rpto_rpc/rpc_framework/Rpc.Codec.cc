#include "Rpc.Codec.h"

#include "TcpConnection.h"
#include "Buffer.h"
#include "Endian.h"

#include <google/protobuf/message.h>
#include <zlib.h>


using namespace network;

namespace network {

/**
 * @brief 发送数据
 * @param conn 连接
 * @param message 消息
 */
void ProtoRpcCodec::send(const TcpConnectionPtr& conn, const golgmessage& message) {
    Buffer buf;
    fillEmptyMessage(&buf, message);
    conn->send(&buf);
}

/**
 * @brief 将message填充到buf中
 * @param buf 缓冲区
 * @param message 消息
 */
void ProtoRpcCodec::fillEmptyMessage(Buffer* buf, const golgmessage& message) {
    assert(buf->readableBytes() == 0);
    buf->append(tag_);      //"RPC0"

    int byte_size = serializeToBuffer(message, buf);//写入缓冲区的大小

    int32_t checkSum = checksum(buf->peek(), static_cast<int>(buf->readableBytes()));
    buf->appendInt32(checkSum);
    //可读数据是 tag 加上 byte_szie 加上 checksum
    assert(buf->readableBytes() == tag_.size() + byte_size +KChecksumLen);
    (void)byte_size;
    int32_t len = endian::host_to_net_32(static_cast<int32_t>(buf->readableBytes()));
    buf->prepend(&len, sizeof(len));
}

/**
 * @brief 在所有检查无误后，调用messagecb_处理消息
 * @param conn 连接
 * @param buffer 缓冲区
 */
void ProtoRpcCodec::onMessage(const tcpConnectionPtr& conn, Buffer* buffer) {
    while(buffer-> readableBytes() >= static_cast<uint32_t>(KChecksumLen + KHeaderLen)) {
        const int32_t len = buffer->peekInt32();
        if(len > KMaxMessageLen || len < KMinMessageLen) {
            break;
        } else if(buffer->readableBytes() >= size_t(KHeaderLen + len)) {
            RpcMessagePtr message(new RpcMessage());
            ErrorCode errorCode = parse(buffer->peek() + KHeaderLen, len, message.get());

            if(errorCode == ErrorCode::KNoError) {
                //无错误
                messagecb_(conn, message);
                buffer->retrieve(KHeaderLen + len);
            } else {
                break;
            }
        } else {
            break;
        }
    }
}

/**
 * @brief 解析消息
 * @param buf 待解析的消息
 * @param len 待解析的消息长度
 * @param message 解析后的消息
 * @return ErrorCode 错误码
 */
ErrorCode ProtoRpcCodec::parse(const char* buf, int len, golgmessage* message) {
    ErrorCode errorcode = ErrorCode::KNoError;
    if(vailddataChecksum(buf, len)) {
        //合法数据
        if(memcmp(buf, tag_.data(), tag_.size())) {
            const char* data = buf + tag_.size();
            int32_t datalen = len - KChecksumLen - static_cast<int32_t>(tag_.size());
            if(parseFromBuffer(data, datalen, message)) {
                //解析数据成功
                errorcode = ErrorCode::KNoError;
            } else {
                errorcode = ErrorCode::KParseEroor;//解析数据错误
            }
        } else {
            errorcode = ErrorCode::KUnKnownMessageType; //未知消息类型
        }
    } else {
        errorcode = ErrorCode::KcheckSumError; //校验和错误
    }
    return errorcode;
}

/**
 * @brief 解析序列化消息
 * @param buf 待解析的消息
 * @param len 待解析的消息长度
 * @param message 解析后的消息
 * @return bool 解析成功或失败
 */
bool ProtoRpcCodec::parseFromBuffer(const void* buf, int len, golgmessage* message) {
    //包装了protobuf的解析方法
    return message->ParseFromArray(buf, len);
}

/**
 * @brief 序列化消息
 */
int ProtoRpcCodec::serializeToBuffer(const golgmessage& message, Buffer* buf) {

#if GOOGLE_PROTOBUF_VERSION >= 3009002
    /**/
    int byte_size = google::protobuf::internal::ToIntSize(message.ByteSizeLong());
#else
    int byte_size = message.ByteSize();
#endif

    //确保有足够的空间（KChecksumLen + 序列化消息的长度）
    buf->ensureWriteableBytes(byte_size + KChecksumLen);

    uint8_t* start = reinterpret_cast<uint8_t*>(buf->beginWrite());
    /**/
    uint8_t* end = message.SerializeWithCachedSizesToArray(start);
    
    //检查写入的字节数是否与消息的字节数一致
    if(end - start != byte_size) {
        //do failed
    }

    buf->hasWritten(byte_size);
    return byte_size;
}


int32_t ProtoRpcCodec::asInt32(const char* buf) {
    int32_t be32 = 0;
    ::memcpy(&be32, buf, sizeof(be32));
    return endian::net_to_host_32(be32);
}

int32_t ProtoRpcCodec::checksum(const void* buf, int len) {
    /*adler32y与CRC-32sh算法的区别在于，adler32y是对数据流的校验，而CRC-32sh是对数据流的字节级校验。*/
    /*adler-32是一种校验算法，它通过累加数据流的每个字节的校验和来计算校验值。*/
    return static_cast<int32_t>(::adler32(1, static_cast<const Bytef*>(buf), len));
}

bool ProtoRpcCodec::vailddataChecksum(const char* buf, int len) {
    int32_t expectedCheckSum = asInt32(buf + len - KChecksumLen);
    int32_t actualCheckSum = checksum(buf, len - KChecksumLen);
    return expectedCheckSum == actualCheckSum;
}

}
