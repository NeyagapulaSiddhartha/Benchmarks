#include "/home/sidda/FINDUS_Artifact/FINDUS/Benchmarks/MTUASBench/Utilites/doWork.h"
#include <iostream>
#include <pthread.h>
#include <unistd.h>

class A {
    public:
    int a=10;
    virtual void m1() {
        std::cout << "A m1"<<"Data:"<<a<<"\n";
    }
    void m2() {
        std::cout << "A m2"<<"Data:"<<a<<"\n";
    }
};

class A1 : public A {
    public:
    int a1=20;
    void m1() {
        std::cout << "A1 m1" << "Data:"<<a1<<"\n";
    }
};

class A2 : public A {
    public:
    int a2=30;
    void m2() {
        std::cout << "A2 m2" << "Data:"<<a2<<"\n";
    }
};

pthread_t t1;

void* threadFunction(void* arg) {
    A2* a2 = static_cast<A2*>(arg);
    A1* a1 = dynamic_cast<A1*>(a2);
    
    if (a1) {
        a1->m1();
    }
    return nullptr;
}  

void createThread() {   
    A2 a2;
    pthread_create(&t1, nullptr, threadFunction, &a2);
}  

int main() {
    createThread();
    pthread_join(t1, nullptr);
    std::cout << "Main thread finishes.\n";
    return 0;
}
