#include "/home/sidda/FINDUS_Artifact/FINDUS/Benchmarks/MTUASBench/Utilites/doWork.h"
/* Description: Thread creation inside a function createThread(). Local variable is passed as reference to 
   thread running the function task. As the thread is joined at wrong location (inside main) it creates a Use After 
   Scope bug */

#include <iostream>
#include <pthread.h>

pthread_t t;
void* task(void* arg) {
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

void createThread() {
    int localVar = 5;
    
    std::cout << "Before thread, localVar: " << localVar << "\n";
    
    pthread_create(&t, nullptr, task, &localVar);  // Create thread and pass the address of localVar
    
    // Join the thread inside the createThread function to avoid scope issues
    
       for(int i=0;i<5;i++)
       {
           for(int j=0;j<5;j++)
           {
               std::cout<<"doing some work in thread "<<"\n";
               doWork(1000);
           }
            doWork(200);

       }
    
    std::cout << "After thread, localVar: " << localVar << "\n";
}

int main() {
    createThread();  // Create the thread and join inside createThread()
    pthread_join(t, nullptr);
    return 0;
}


