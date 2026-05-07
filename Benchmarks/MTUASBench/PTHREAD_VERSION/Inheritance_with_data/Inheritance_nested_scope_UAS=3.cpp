#include "/home/sidda/FINDUS_Artifact/FINDUS/Benchmarks/MTUASBench/Utilites/doWork.h"
#include <iostream>
#include <pthread.h>
#include <unistd.h>

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

A* basePtr;
pthread_t t1, t2,t3;

void* runThread1(void* arg) {
    
    for(int i=0;i<10;i++) // Simulating some work in the main thread
        {
            for(int j=0;j<10;j++)
            {
                A* basePtr = static_cast<A*>(arg);
                basePtr->m1();
            }
            doWork(1000);
        }    
    return nullptr;
}

void* runThread2(void* arg) {
       
    for(int i=0;i<10;i++) // Simulating some work in the main thread
        {
            for(int j=0;j<10;j++)
            {
                 A* basePtr = static_cast<A*>(arg);
                basePtr->m2();
            }
            doWork(1000);
        }  
   
    return nullptr;
}

void createThread() {
    int x = 9;

    {
        A2 a2Obj;
        basePtr = &a2Obj;
        pthread_create(&t1, nullptr, runThread2, basePtr);

        if (x >= 9){
            A1 a1Obj;
            basePtr = &a1Obj;
            pthread_create(&t2, nullptr, runThread1, basePtr);
            if (x >= 8) {
                A1 a2Obj;
                basePtr = &a2Obj;
                pthread_create(&t3, nullptr, runThread1, basePtr);


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
        }
        std::cout << "Inner scope ends" << std::endl;
            pthread_join(t2, nullptr);

    }
    pthread_join(t1, nullptr);
    
}

int main() {
    createThread();
    pthread_join(t3, nullptr);
    return 0;
}
