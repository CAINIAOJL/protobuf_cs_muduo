#include <iostream>  
#include <fstream>  
#include "person.pb.h"  
  
int main() {  
    // 创建一个 Person 对象并设置其字段  
    Person person;  
    person.set_name("John Doe");  
    person.set_id(1234);  
    person.set_email("johndoe@example.com");  
  
    // 序列化到文件  
    std::ofstream output("person.bin", std::ios::binary);  
    if (!person.SerializeToOstream(&output)) {  
        std::cerr << "Failed to write person." << std::endl;  
        return -1;  
    }  
    output.close();  
  
    // 从文件反序列化  
    Person deserialized_person;  
    std::ifstream input("person.bin", std::ios::binary);  
    if (!deserialized_person.ParseFromIstream(&input)) {  
        std::cerr << "Failed to parse person." << std::endl;  
        return -1;  
    }  
    input.close();  
  
    // 打印反序列化后的对象  
    std::cout << "Name: " << deserialized_person.name() << std::endl;  
    std::cout << "ID: " << deserialized_person.id() << std::endl;  
    std::cout << "Email: " << deserialized_person.email() << std::endl;  
  
    return 0;  
}

/*命令*/
// g++ -o target xxxx.cc xxxxx.pb.cc -std=c++11 -lprotobuf