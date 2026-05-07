#include "/home/sidda/FINDUS_Artifact/FINDUS/Benchmarks/MTUASBench/Utilites/doWork.h"
#include <iostream>
#include <pthread.h>
#include <unistd.h>
using namespace std;
// thread t1;

class Base {
    public:
        int a=10;
         void func()
        {
            std::cout<<"Base"<<"\n";

        }
    
    };
    
    Base* basePtr;
    
    class Derived : public Base {
    public:
        int b=20;
        void func() 
        {
            std::cout<<"Derived:"<<b<<"\n";
        }
      
    };


int main()  
{   
    
    Derived derivedObj; 
   // basePtr = &derivedObj;    
    // pthread_create(&t1,NULL,func1,&derivedObj);
    // pthread_join(t1,NULL);
   //    std::cout<<basePtr->a<<"\n";
       return 0;
}

