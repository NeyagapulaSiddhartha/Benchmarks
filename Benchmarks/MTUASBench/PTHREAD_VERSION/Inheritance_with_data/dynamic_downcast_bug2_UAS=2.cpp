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
    
    A* basePtrA1;
    A* basePtrA2;
    
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

pthread_t t1, t2;

void* runThread1(void* arg) {
    A* basePtr = static_cast<A*>(arg);
    basePtr->m1();
    A1* a1Ptr = dynamic_cast<A1*>(basePtr);
    if (a1Ptr) {
        a1Ptr->m1();
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
                    A2* a2Ptr = dynamic_cast<A2*>(basePtr);
                    if (a2Ptr) {
                        a2Ptr->m2();
                    }
            }
            doWork(1000);
        }    
    return nullptr;
}

void createThread() {
    A1 a1Obj;
    A2 a2Obj;

    basePtrA1 = &a1Obj;
    basePtrA2 = &a2Obj;

    pthread_create(&t1, nullptr, runThread1, basePtrA1);
    pthread_join(t1, nullptr);
    
    pthread_create(&t2, nullptr, runThread2, basePtrA2);

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

            doWork(3000); // Sleep for 0.5 seconds

    pthread_join(t2, nullptr);
    return 0;
}
