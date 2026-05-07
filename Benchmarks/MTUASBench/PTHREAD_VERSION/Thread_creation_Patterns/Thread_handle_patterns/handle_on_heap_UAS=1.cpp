#include "/home/sidda/FINDUS_Artifact/FINDUS/Benchmarks/MTUASBench/Utilites/doWork.h"
/* Description: Thread handle created on heap using new keyword in main function. 
Same handle used to run threadFunction passing counter as reference */

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

int main() {
    int counter = 5;

    pthread_t* t = new pthread_t;  // Create a new thread handle

    // Create a new thread
    pthread_create(t, nullptr, threadFunction, &counter);
       for(int i=0;i<5;i++)
       {
           for(int j=0;j<5;j++)
           {
               std::cout<<"doing some work in thread "<<"\n";
               doWork(1000);
           }
            doWork(200);

       }
    // Ensure the thread is joined before proceeding
    // if (pthread_join(*t, nullptr) != 0) {
    //     std::cerr << "Error joining thread\n";
    // }

    delete t;  // Clean up dynamically allocated thread handle

    std::cout << "Main function ends\n";
    return 0;
}
