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

    for(int i=0;i<10;i++) // Simulating some work in the main thread
        {
            for(int j=0;j<10;j++)
            {
                    Base* baseRef = static_cast<Base*>(arg);
                    baseRef->display();
            }
            doWork(1000);
        }


    return nullptr;
}  

void createThread() {   
    Derived derivedObj; 
    basePtr = &derivedObj;    
    pthread_create(&t1, nullptr, threadFunction, basePtr);


        for(int i=0;i<10;i++) // Simulating some work in the main thread
        {
            for(int j=0;j<10;j++)
            {
                auto temp =i+j;

                std::cout<<"Main thread working...\t"<<temp<<"\n";
            }
            doWork(1000);
        }
}  

int main() {
    createThread();
    pthread_join(t1, nullptr);
    std::cout << "Main thread finishes.\n";
    return 0;
}