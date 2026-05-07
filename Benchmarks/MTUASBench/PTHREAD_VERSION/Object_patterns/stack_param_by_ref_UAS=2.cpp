#include "/home/sidda/FINDUS_Artifact/FINDUS/Benchmarks/MTUASBench/Utilites/doWork.h"
/* Description: Object x is created on stack (local var) and then passed as reference to thread function thread_func. Here the thread t1 is joined in main function instead of 
   threadFunction which leads to Use After Scope bug for data object x */

#include <iostream>
#include <pthread.h>
#include<unistd.h>
pthread_t t1,t2;


   void* thread_func(void *arg) {


     for(int i=0;i<5;i++)
       {
           for(int j=0;j<5;j++)
           {
            
            int *x = static_cast<int*>(arg);
            std::cout << "Value of x is:" << *x << "\n";
            doWork(1000);
           
        }
       }

       
       // std::cout<<"Value of x is:";
       return nullptr;
   }

void threadFunction() {
    int x =10;
    int y=20;

    pthread_create(&t1, nullptr, thread_func, static_cast<void*>(&x));


             




    pthread_create(&t2, nullptr, thread_func, static_cast<void*>(&y));


      
            std::cout<<" the work is being done \n";
           

       for(int i=0;i<6;i++)
       {
           for(int j=0;j<5;j++)
           {
               std::cout<<"doing some work in thread "<<"\n";
               doWork(1000);
           }
                   doWork(200);

       }
           



    doWork(1000); 

}

int main() {
    //  int x=10;
    // t1=std::thread(thread_func);
    threadFunction();
    pthread_join(t1, nullptr);
    pthread_join(t2, nullptr);
}
