#include <iostream>
#include "echo.pb.h"

void test_relection() {
  const std::string type_name = "self.EchoRequest";
  /*
   * ① 在 DescriptorPool 中检索 self.EchoRequest
   *    Message 类型的 discriptor 元数据
   */ 
  const google::protobuf::Descriptor* descriptor 
    = google::protobuf::DescriptorPool::generated_pool()
      ->FindMessageTypeByName(type_name);
  if (descriptor == nullptr) {
    std::cout << "[ERROR] Cannot found " << type_name
          << " in DescriptorPool" << std::endl;
    return;
  }
  /*
   * ② 通过 discriptor 元信息在 MessageFactory 检索类型工厂，
   *    用于创建该类型的实例。
   */
  const google::protobuf::Message* prototype
    = google::protobuf::MessageFactory::generated_factory()
      ->GetPrototype(descriptor);
  /*
   * ③ 创建 self.EchoRequest 类型的 Message 实例。
   *    google::protobuf::Message 是所有 Message
   *    类型的基类。
   */
  google::protobuf::Message* req_msg = prototype->New();
  /*
   * ④ 因为只知道基类的实例指针，需要 Reflection 信息协助判断
   *    具体类型。
   */
  const google::protobuf::Reflection* req_msg_ref
    = req_msg->GetReflection();
  /*
   * ⑤ 作为开发者，是知道该 Message 是对应哪个类型的，但是程序不知道，
   *    开发者告诉程序，试着获取其 payload 字段。
   */
  const google::protobuf::FieldDescriptor *req_msg_ref_field_payload
    = descriptor->FindFieldByName("payload");

  /*
   * ⑥ Field 信息 + Reflection 信息配合读取 payload 的数据。
   */
  std::cout << "before set, ref_req_msg_payload: "
            << req_msg_ref->GetString(*req_msg, req_msg_ref_field_payload)
            << std::endl;
  /*
   * ⑦ Field 信息 + Reflection 信息配合写入 payload 的数据。
   */
  req_msg_ref->SetString(req_msg, req_msg_ref_field_payload, "my payload");
  /*
   * ⑧ Field 信息 + Reflection 信息配合再次读取 payload 的数据。
   */
  std::cout << "after set, ref_req_msg_payload: "
            << req_msg_ref->GetString(*req_msg, req_msg_ref_field_payload)
            << std::endl;
}
int main() {
    self::EchoRequest req;
    test_relection();
    return 0;
}