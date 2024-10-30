#include <iostream>
#include <memory>
using namespace std;

class myclass {
public:
    myclass() = default;
    myclass(const int count) {
        count_ = count;
    }

    ~myclass() {}
    void display() {
        cout << "count = " << count_ << endl;
    }
private:
    int count_;
};


int main() {
    /*shared_ptr<myclass> p1(new myclass(122));
    shared_ptr<myclass> p2;
    p2 = p1;

    cout << "p1.use_count() = " << p1.use_count() << endl;
    cout << "p2.use_count() = " << p2.use_count() << endl;*/

    shared_ptr<myclass> p1(new myclass(11));
    weak_ptr<myclass> p2 = p1;

    cout << "shared_ptr ---->" << "p1.use_count() = " << p1.use_count() << endl;
    //cout << "weak_ptr   ---->" << "p2.use_count() = " << p2.use_count() << endl;

    cout << "create a new shared_ptr -> p1 \n";
    shared_ptr<myclass> p3 = p1;

    cout << "shared_ptr ---->" << "p1.use_count() = " << p1.use_count() << endl;
    //cout << "weak_ptr   ---->" << "p2.use_count() = " << p2.use_count() << endl;
    cout << "shared_ptr ---->" << "p3.use_count() = " << p3.use_count() << endl;

    cout << "delete p1 \n";
    p1.reset();

    cout << "shared_ptr ---->" << "p1.use_count() = " << p1.use_count() << endl;
    //cout << "weak_ptr ---->" << "p2.use_count() = "   << p2.use_count() << endl;
    cout << "shared_ptr ---->" << "p3.use_count() = " << p3.use_count() << endl;

    //cout << "delete p3 \n";
    //p3.reset();

    //cout << "shared_ptr ---->" << "p1.use_count() = " << p1.use_count() << endl;
    //cout << "weak_ptr   ---->" << "p2.use_count() = " << p2.use_count() << endl;
    //cout << "shared_ptr ---->" << "p3.use_count() = " << p3.use_count() << endl;

    cout << "try to get ptr from weak_ptr \n";
    shared_ptr<myclass> p4 = p2.lock();
    if (p4 == nullptr) {  
        cout << "The class is still alive, but not owned by p4." << endl;  
    } else {  
        cout << "The class is still alive and owned by p4." << endl;  
        p4->display();  
    } 

    return 0;
}