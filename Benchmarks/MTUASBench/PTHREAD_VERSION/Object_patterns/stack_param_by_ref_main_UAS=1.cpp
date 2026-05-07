#include "/home/sidda/FINDUS_Artifact/FINDUS/Benchmarks/MTUASBench/Utilites/doWork.h"
/* Description: Object x is created on stack (local var) and then passed as reference to thread function thread_func.
The thread is joined outside the scope of object creation which results in UAS bug*/

#include <iostream>
#include <pthread.h>
#include <bits/stdc++.h>
#include <unistd.h>
using namespace std;

pthread_t t1;

void* thread_func(void *arg) {

        

            for(int j=0;j<5;j++){

                  for(int jj=0;jj<5;jj++){
                     int *y = static_cast<int*>(arg);
                     // cout<<"Mem loc:"<<y<<endl;
                         std::cout << "Value of y is:" << *y << "\n";
                        std::cout<<"doing some work in main "<<"\n";
                     doWork(1000);
                  }
            }
    return nullptr;
}

int main() {
    int a=56;
    // std::cin >> a;
    
    if (a == 56) {
        int x = 10;
        // cout<<"Mem loc:"<<&x<<endl;
        pthread_create(&t1, nullptr, thread_func, &x);
       
       for(int i=0;i<5;i++)
       {
           for(int j=0;j<5;j++)
           {
               std::cout<<"doing some work in thread "<<"\n";
               doWork(1000);
           }
            doWork(200);

       }

    }
    std::cout << "Scope over" << std::endl;
    
    pthread_join(t1, nullptr);
}


/* Description: Object x is created on stack (local var) and then passed as reference to thread function thread_func. Here the thread t1 is joined in main function instead of 
   threadFunction which leads to Use After Scope bug for data object x */


   