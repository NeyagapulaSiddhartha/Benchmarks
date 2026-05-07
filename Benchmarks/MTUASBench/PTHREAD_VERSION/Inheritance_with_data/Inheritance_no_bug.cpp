#include "/home/sidda/FINDUS_Artifact/FINDUS/Benchmarks/MTUASBench/Utilites/doWork.h"
#include <iostream>
#include <pthread.h>
#include <unistd.h>

class Base {
    // private:
   
public:
int a=10;
    void display() {
        std::cout << "Base display:" <<a<<"\n";
    }
};

Base* basePtr;

class Derived : public Base {
public:
    int b=20;
    void display() {
        std::cout << "Derived display" <<b<< "\n";
    }
};

pthread_t t1;

void* threadFunction(void* arg) {
    Base* baseRef = static_cast<Base*>(arg);
    baseRef->display();
    return nullptr;
}  

void createThread() {   
    Derived derivedObj; 
    basePtr = &derivedObj;    
    pthread_create(&t1, nullptr, threadFunction, basePtr);
    pthread_join(t1, nullptr);
}  

int main() {
    createThread();
   
    std::cout << "Main thread finishes.\n";
    return 0;
}