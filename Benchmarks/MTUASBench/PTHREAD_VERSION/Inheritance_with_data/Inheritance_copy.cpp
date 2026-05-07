#include "/home/sidda/FINDUS_Artifact/FINDUS/Benchmarks/MTUASBench/Utilites/doWork.h"
#include <iostream>
#include <thread>
#include <unistd.h>
using namespace std;
thread t1;

class Base {
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

// void threadFunction(Base *baseRef) {
//     baseRef->display();
// }  
void createThread()  
{  
    Derived derivedObj; 
    basePtr = &derivedObj;    
    // t1=thread(threadFunction,basePtr);
     //t1.join();
       basePtr->display();
}
int main() {
    createThread();
    // t1.join();
    cout << "Main thread finishes.\n";
    return 0;
}
