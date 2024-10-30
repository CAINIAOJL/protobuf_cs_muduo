#pragma once
namespace network {
class Nocopyable {
    public:
        /*禁止拷贝构造函数和赋值运算符*/
        nocopyable(const nocopyable&) = delete;
        nocopyable& operator=(const nocopyable&) = delete;
    protected:
        nocopyable() = default;
        ~nocopyable() = default;
};
}
