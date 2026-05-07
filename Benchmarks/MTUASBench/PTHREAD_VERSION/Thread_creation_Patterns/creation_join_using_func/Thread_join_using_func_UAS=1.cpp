
#include "/home/sidda/FINDUS_Artifact/FINDUS/Benchmarks/MTUASBench/Utilites/doWork.h"
/* Description: Passing thread handle as parameter to join the thread.
   Here joinThread function is called inside main(), which may lead to potential Use After Scope issues. */

#include <iostream>
#include <pthread.h>
#include <unistd.h>
// Global thread handle
pthread_t t1;

// Function that will be executed by the thread
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

// Function that takes a thread handle and joins the thread
void joinThread(pthread_t t) {


    std::cout << "Joining thread...\n";
    pthread_join(t, nullptr);  // Join the thread to ensure it finishes
    std::cout << "Thread joined successfully.\n";
    
}

void createThread(int x) {
   
    // Create a thread and pass the address of x
    pthread_create(&t1, nullptr, threadTask, &x);
  
       for(int i=0;i<5;i++)
       {
           for(int j=0;j<5;j++)
           {
               std::cout<<"doing some work in thread "<<"\n";
               doWork(1000);
           }
            doWork(200);

       }
  
    std::cout << "Function createThread went out of scope.\n";

}

int main() {



     int x = 100;
    createThread(x);
    // Pass the thread handle (t1) to another function to join it
    joinThread(t1);  // Passing the thread handle as a reference
    return 0;



}
