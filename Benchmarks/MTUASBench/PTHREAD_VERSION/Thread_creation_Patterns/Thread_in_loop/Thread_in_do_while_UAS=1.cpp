#include "/home/sidda/FINDUS_Artifact/FINDUS/Benchmarks/MTUASBench/Utilites/doWork.h"
/* Description: 50 threads are created in do while loop. First 40 threads are created in else block and last 10 in if block. The threads are joined outside the 
if-else block leading to Use After Scope bug */

#include <iostream>
#include <pthread.h>
#include <vector>
#include <unistd.h>

using namespace std;
std::vector<pthread_t> threads; // Vector to store thread handles
int tcount = 0;
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

void* threadTask2(void* arg) {

            for(int j=0;j<5;j++){

                  for(int jj=0;jj<5;jj++){
                     int *y = static_cast<int*>(arg);
                     // cout<<"Mem loc:"<<y<<endl;
                         std::cout << "Value of y is:" << *y << "\n";
                        std::cout<<"doing some work in main "<<"\n";
                  }
                                       doWork(700);
            }
    return nullptr;
}

int spawnThread() {
   
    int sharedData = 2;

    // Create thread chain with data dependency
    do {
        pthread_t tid; // Temporary thread handle
        if (tcount > 40) {
           
            pthread_create(&tid, nullptr, threadTask2, &sharedData);
            threads.push_back(tid);
            tcount++;
        } else {
          
            pthread_create(&tid, nullptr, threadTask, &sharedData);
            threads.push_back(tid);
            tcount++;
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

    } while (tcount < 4);

    cout << "Loop completed !!!" << endl;

  

    return 0;
}

int main()
{
    spawnThread();
    for (int i = 0; i < tcount; i++) {
        std::cout << "Thread " << i + 1 << " is joining" << endl;
        pthread_join(threads[i], nullptr); // Join all threads
    }
    return 0;
}
