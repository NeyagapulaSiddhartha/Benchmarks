#include "/home/sidda/FINDUS_Artifact/FINDUS/Benchmarks/MTUASBench/Utilites/doWork.h"
/* Description: 50 threads are created in do while loop. First 40 threads are created in else block and last 10 in if block. The threads are joined immediately
inside the if-else block avoiding the potential Use After Scope bugs */

#include <iostream>
#include <pthread.h>
#include <vector>

using namespace std;

// Thread function for task 1 (sharedData is passed by pointer)
void* threadTask(void* arg) {
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

// Thread function for task 2 (sharedData is passed by value)
void* threadTask2(void* arg) {
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
    std::vector<pthread_t> threads;
    int i = 2;
    int id = 1;

    // Create thread chain with data dependency
    do {
        int sharedData=100;
        pthread_t tid;
        if (i > 1 && i < 10) {
            // Pass sharedData by value for threadTask2
            pthread_create(&tid, nullptr, threadTask2, &sharedData);
            threads.push_back(tid);
            
        } else {
            // Pass sharedData by pointer for threadTask
            pthread_create(&tid, nullptr, threadTask, &sharedData);
            std::cout << tid << " is joining" << std::endl;
            pthread_join(tid, nullptr); // Join immediately inside the else block
        }


       for(int i=0;i<5;i++)
       {
           for(int j=0;j<5;j++)
           {
               std::cout<<"doing some work in thread "<<"\n";
               doWork(900);
           }
            doWork(100);

       }
        i++;
    } while (i < 3);

    for (size_t i = 2; i < threads.size(); i++) {
        std::cout << threads[i] << " is joining" << std::endl;
        pthread_join(threads[i], nullptr);
    }
       for(int i=0;i<5;i++)
       {
           for(int j=0;j<5;j++)
           {
               std::cout<<"doing some work in thread "<<"\n";
               doWork(1000);
           }
            doWork(200);

       }
    std::cout << "Final sharedData: " << i << "\n";
    return 0;
}
