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
    
    class A1 : virtual public A {
    public:
    int a1=20;
        void m1() override {
            std::cout << "A1 class m1:"<<a1<<"\n";
        }
    };
    
    class A2 : virtual public A {
    public:
    int a2=30;
        void m2() {
            std::cout << "A2 class m2:"<<a2<<"\n";
        }
    };
    
    class A12 : public A1, public A2 {
    public:
    int a12=40;
        void m1() override {
            std::cout << "A12 class m1:"<<a12<<"\n";
        }
    };
    

pthread_t t1;

void* runThread(void* arg) {

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

void createThread() {
    A12 a12Obj;
    A12* basePtr = &a12Obj;
    pthread_create(&t1, nullptr, runThread, basePtr);

    // doWork(100000); // Simulating some work in the main thread

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
    return 0;
}
