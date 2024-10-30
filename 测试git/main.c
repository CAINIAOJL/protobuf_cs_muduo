#include <stdio.h>

// 声明 C++ 函数（使用 extern "C" 进行 C 和 C++ 互操作）
#ifdef __cplusplus
extern "C" {
#endif
    const char* concat_strings(const char* a, const char* b);
    const char* get_current_time();
#ifdef __cplusplus
}
#endif

int main() {
    const char* result = concat_strings("Hello, ", "Abseil!");
    printf("Concatenated String: %s\n", result);

    const char* time_str = get_current_time();
    printf("Current Time: %s\n", time_str);

    return 0;
}

