#include "/home/sidda/FINDUS_Artifact/FINDUS/Benchmarks/MTUASBench/Utilites/doWork.h"
/* Description: Thread handle is passed by reference to a function startThread which uses the same handle
to create thread running threadFunction */

#include <iostream>
#include <pthread.h>

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

void runThread(pthread_t *t, int counter) {
    pthread_create(t, nullptr, threadFunction, &counter);  // Create a new thread
    // pthread_join(*t, nullptr);  // Wait for the thread to finish


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

int startThread() {
    pthread_t t;  // Thread handle
    int counter = 5;
    runThread(&t, counter);  // Pass thread handle by reference
    pthread_join(t, nullptr);  // Wait for the thread to finish
    std::cout << "Main function ends\n";
    return 0;
}

int main() {
    startThread();
    return 0;
}
