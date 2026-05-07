#include "/home/sidda/FINDUS_Artifact/FINDUS/Benchmarks/MTUASBench/Utilites/doWork.h"
#include <iostream>
#include <pthread.h>
#include <unistd.h>

class Base1 {
    public:
    int b1=10;
        void display() {
            std::cout << "Base1 display:"<<b1<<"\n";
        }
    };
    
    class Base2 {
    public:
    int b2=20;
        virtual void display() {
            std::cout << "Base2 display:"<<b2<<"\n";
        }
    };
    
    class Derived1 : public Base1 {
    public:
        int d1=30;
        void display() {
            std::cout << "Derived1 display"<<d1<<"\n";
        }
    };
    
    class Derived2 : public Base2 {
    public:
        int d2=40;
        void display() override {
            std::cout << "Derived2 display:"<<d2<<"\n";
        }
    };

Derived2* derived2Ptr;
pthread_t t1;

void* threadFunction(void* arg) {
    Derived2* derivedRef = static_cast<Derived2*>(arg);
    derivedRef->display();
    return nullptr;
}

void createThread() {
    Derived1 derived1Obj;
    Derived2 derived2Obj;
    derived2Ptr = reinterpret_cast<Derived2*>(&derived1Obj);
    pthread_create(&t1, nullptr, threadFunction, derived2Ptr);
    pthread_join(t1, nullptr);
}

int main() {
    createThread();
    std::cout << "Main thread finishes.\n";
    return 0;
}