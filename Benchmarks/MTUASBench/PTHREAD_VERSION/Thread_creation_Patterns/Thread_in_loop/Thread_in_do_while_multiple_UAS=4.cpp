#include "/home/sidda/FINDUS_Artifact/FINDUS/Benchmarks/MTUASBench/Utilites/doWork.h"
/* Description: 50 threads are created in do while loop. First 40 threads are created in else block and last 10 in if block. The threads are joined immediately
inside the if-else block avoiding the potential Use After Scope bugs */

#include <iostream>
#include <pthread.h>
#include <vector>
#include <unistd.h>

using namespace std;

// Thread function for task 1 (sharedData is passed by pointer)
void* threadTask(void* arg) {
            for(int j=0;j<5;j++){

                  for(int jj=0;jj<15;jj++){
                     int *y = static_cast<int*>(arg);
                     // cout<<"Mem loc:"<<y<<endl;
                         std::cout << "Value of y is:" << *y << "\n";
                        std::cout<<"doing some work in ################################## "<<"\n";
                     doWork(1000);
             
                    }
                                         doWork(200);
            }
    return nullptr;
}

// Thread function for task 2 (sharedData is passed by value)
void* threadTask2(void* arg) {

            for(int j=0;j<5;j++){

                  for(int jj=0;jj<15;jj++){
                     int *y = static_cast<int*>(arg);
                     // cout<<"Mem loc:"<<y<<endl;
                         std::cout << "Value of y is:" << *y << "\n";
                        std::cout<<"doing some work in main "<<"\n";
                     doWork(1000);
                  }
            }
    return nullptr;
}
pthread_t t1,t2;
int main() {


    int temp =99;
    std::vector<pthread_t> threads;

    if(temp>55)
    {
    
            int sharedData = 0;
            int id = 1;
           
        if(temp>0)
       {
         pthread_create(&t1, nullptr, threadTask2, &id);
       
       }

    // Create thread chain with data dependency
    do {
        pthread_t tid;
        if (sharedData > 1 && sharedData < 10) {
            // Pass sharedData by value for threadTask2
            pthread_create(&tid, nullptr, threadTask2, &sharedData);
            threads.push_back(tid);
        } else {
            // Pass sharedData by pointer for threadTask
            pthread_create(&tid, nullptr, threadTask, &sharedData);
            std::cout << tid << " is joining" << std::endl;
            
            // pthread_join(tid, nullptr); // Join immediately inside the else block
        }
                    sharedData++;

    } while (sharedData < 4);

        if(temp>0)
       {
        
         pthread_create(&t2, nullptr, threadTask2, &id);

       }
       for(int i=0;i<5;i++)
       {
           for(int j=0;j<5;j++)
           {
               std::cout<<"doing some work in thread "<<"\n";
               doWork(1000);

           }
               doWork(100);

       }



    std::cout << "Final sharedData: " << sharedData << "\n";
    }



        for (size_t i = 2; i < threads.size(); i++) {
        std::cout << threads[i] << " is joining" << std::endl;
        pthread_join(threads[i], nullptr);
    }
    return 0;
}
