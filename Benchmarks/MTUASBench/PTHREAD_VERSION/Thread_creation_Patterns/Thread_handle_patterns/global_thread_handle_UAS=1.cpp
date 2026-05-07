#include "/home/sidda/FINDUS_Artifact/FINDUS/Benchmarks/MTUASBench/Utilites/doWork.h"
/* Description: Thread handle is initialized globally. Thread is created in function startThread 
running thread function where local value is passed by reference. This is buggy program as the thread join 
is placed in main function by which the scope of startThread function ends much before */

#include <iostream>
#include <pthread.h>

// Global thread handle
pthread_t t;

// Thread function
void* threadFunction(void* arg) {
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

// Function where the thread is created
void startThread() {
    int counter = 0;
    pthread_create(&t, nullptr, threadFunction, &counter);  // Pass counter by pointer

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

int main() {
    startThread();
    pthread_join(t, nullptr);  // Ensure the thread is joined before main continues
    return 0;
}
