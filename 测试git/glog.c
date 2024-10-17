#include <glog/logging.h>

int main(int argc, char* argv[]) {  
    // 初始化 glog，通常传递程序名作为参数  
    google::InitGoogleLogging(argv[0]);  
  
    // 设置日志输出目的地（可选，默认输出到标准输出）  
    // google::SetLogDestination(google::GLOG_INFO, "./my_info_log");  
    // google::SetLogDestination(google::GLOG_WARNING, "./my_warning_log");  
    // ... 可以为其他级别设置日志输出  
  
    // 记录不同级别的日志信息  
    LOG(INFO) << "This is an informational message.";  
    LOG(WARNING) << "This is a warning message.";  
    LOG(ERROR) << "This is an error message.";  
    LOG(FATAL) << "This is a fatal message. The program will terminate after this.";  
  
    // 注意：FATAL 级别的日志会导致程序终止，因此下面的代码不会被执行。  
    // 如果您想继续执行程序，请不要使用 FATAL 级别的日志，或者在使用前添加条件判断。  
  
    // 通常情况下，您会在程序的末尾关闭 glog  
    google::ShutdownGoogleLogging();  
  
    // 由于 FATAL 日志会导致程序终止，下面的代码实际上不会被执行到。  
    // 但为了完整性，这里还是展示了如何正确地关闭日志系统。  
    // 在实际使用中，如果不需要 FATAL 日志，可以将其注释掉或删除。  
  
    return 0;  
}

/*命令*/
//g++ xxx.cc -o target -lglog