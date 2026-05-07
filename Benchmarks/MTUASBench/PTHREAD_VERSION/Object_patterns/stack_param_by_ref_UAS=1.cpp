#include "/home/sidda/FINDUS_Artifact/FINDUS/Benchmarks/MTUASBench/Utilites/doWork.h"
// /* Description: Object x is created on stack (local var) and then passed as reference to thread function thread_func. Here the thread t1 is joined in main function instead of 
//    threadFunction which leads to Use After Scope bug for data object x */


   #include <iostream>
   #include <pthread.h>
   #include <cassert>
   #include <iostream>
   #include<unistd.h>

   
   pthread_t t1;

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
        int x = 10;
       pthread_create(&t1, nullptr, thread_func, static_cast<void*>(&x));

       for(int i=0;i<5;i++)
       {
           for(int j=0;j<5;j++)
           {
               std::cout<<"doing some work in thread "<<"\n";
               doWork(1000);
           }
                   doWork(200);

       }

    //   int  dd=4;
    //    while(dd--)
    //    {
    //         for(int i=0;i<10;i++)
    //         {
    //             for(int j=0;j<10;j++)
    //             {
    //                 std::cout<<"doing some work in thread "<<"\n";
    //                 doWork(5000);
    //             }
    //         }

    //    }
      
   }
   
   int main() {
       //  int x=10;
       // t1=std::thread(thread_func);
       threadFunction();
       pthread_join(t1, nullptr);
   }