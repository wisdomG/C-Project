#include <iostream>
#include "LeakDetector.h"

#define new new(__FILE__, __LINE__)

class Err {
public:
    Err(int n) {
        if (n == 0) throw 1000;
        //data = new(__FILE__, __LINE__) int[n];
    }
    ~Err() {
        delete[] data;
    }

private:
    int *data;
    double test;
};

int main() {
    int *a = new int[10];
    int *b = new int;

    // windows(minGW)环境下，重载的delete不能执行
    delete(a);
    delete(b);

    try{
        Err *e = new Err(0);
        delete e;
    } catch (int &ex) {
        std::cout << "Exception catch: " << ex << std::endl;
    };

    return 0;
}
