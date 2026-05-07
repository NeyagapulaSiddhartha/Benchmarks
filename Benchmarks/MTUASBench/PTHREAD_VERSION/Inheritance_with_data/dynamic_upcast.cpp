#include "/home/sidda/FINDUS_Artifact/FINDUS/Benchmarks/MTUASBench/Utilites/doWork.h"
#include <iostream>
#include <thread>
#include <unistd.h>
using namespace std;
pthread_t t1, t2;

// Base class A
class A {
    public:
    int a=10;
        virtual void m1() {
            std::cout << "Base class m1:"<<a<<"\n";
        }
        
        void m2() {
            std::cout << "Base class m2:"<<a<<"\n";
        }
    };
    
 
    
    class A1 : public A {
    public:
    int a1=20;
        void m1() override {
            std::cout << "A1 class m1:"<<a1<<"\n";
        }
    };
    
    class A2 : public A {
    public:
    int a2=30;
        void m2() {
            std::cout << "A2 class m2:"<<a2<<"\n";
        }
    };

void* runThread1(void* arg) {
    doWork(5000);
    A* basePtr = static_cast<A*>(arg);
    basePtr->m1();
    A1* a1Ptr = dynamic_cast<A1*>(basePtr);
    // if (a1Ptr) {
        a1Ptr->m1();
    // }
    return nullptr;
}

void* runThread2(void* arg) {
    doWork(5000);
    A* basePtr = static_cast<A*>(arg);
    basePtr->m2();
    A2* a2Ptr = dynamic_cast<A2*>(basePtr);
    // if (a2Ptr) {
        a2Ptr->m2();
    // }
    return nullptr;
}

void createThread() {
    A1 a1Obj;
    A2 a2Obj;

    pthread_create(&t1, nullptr, runThread1, &a1Obj);
    pthread_join(t1, nullptr);
    
    pthread_create(&t2, nullptr, runThread2, &a2Obj);
    pthread_join(t2, nullptr);
}

int main() {
    createThread();
    return 0;
}
