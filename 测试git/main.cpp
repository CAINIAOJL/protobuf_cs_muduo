#include <absl/strings/str_cat.h>
#include <iostream>

int main() {
    std::string result = absl::StrCat("Hello, ", "Abseil!");
    std::cout << result << std::endl;
    return 0;
}

//g++  main.cpp -o example_program -labsl_strings -labsl_base -labsl_synchronization -L/usr/local/lib -I/usr/local/include
//g++  main.cpp -o example_program -labsl_strings -L/usr/local/lib -I/usr/local/include
