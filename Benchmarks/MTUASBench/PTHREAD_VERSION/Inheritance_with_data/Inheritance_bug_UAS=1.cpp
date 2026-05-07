#include "/home/sidda/FINDUS_Artifact/FINDUS/Benchmarks/MTUASBench/Utilites/doWork.h"
#include <iostream>
#include <pthread.h>
#include <unistd.h>
using namespace std;
pthread_t t1;

class Base {
public:
    int a=10;
       virtual void display() {
        cout << "Base display:"<<a<<"\n";
    }
};
Base* basePtr;
class Derived : public Base {
public:
int b=20;
    void display()  {
        cout << "Derived display"<<b<<"\n";
    }
};

void* threadFunction(void* arg) {
    doWork(5000);
    Base* baseRef = static_cast<Base*>(arg);
    baseRef->display();
    return nullptr;
}  

void createThread() {   
    Derived derivedObj; 
    basePtr = &derivedObj;    
    pthread_create(&t1, nullptr, threadFunction, basePtr);
   
}

int main() {
    createThread();
    pthread_join(t1, nullptr);
    std::cout << "Main thread finishes.\n";
    return 0;
}
