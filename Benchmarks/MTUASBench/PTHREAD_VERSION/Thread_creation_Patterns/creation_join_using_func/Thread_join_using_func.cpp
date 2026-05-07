#include "/home/sidda/FINDUS_Artifact/FINDUS/Benchmarks/MTUASBench/Utilites/doWork.h"
/* Passing thread handle as parameter to join the thread.
   Here joinThread function is called in main() function itself which is the owner of the object passed as
   reference avoiding the potential Use After Scope bugs. */

#include <iostream>
#include <pthread.h>
#include <unistd.h>

// Function that will be executed by the thread
void* threadTask(void* id) {
    int* threadId = static_cast<int*>(id);
    std::cout << "Thread " << *threadId << " is running...\n";
    std::cout << "Thread " << *threadId << " has finished.\n";
    return nullptr;
}

// Function that takes a thread handle and joins the thread
void joinThread(pthread_t t) {
    std::cout << "Joining thread...\n";
    pthread_join(t, nullptr);  // Join the thread to ensure it finishes
    std::cout << "Thread joined successfully.\n";
}

int main() {
    int threadId = 1;
    pthread_t t1;  // Declare thread handle

    // Create a thread that runs threadTask
    pthread_create(&t1, nullptr, threadTask, &threadId);

    for(int i = 0; i < 20; i++)
        {
            for(int j = 0; j < 10; j++)
            {
                std::cout << "Doing some work in the main thread...\n";
                        }
        }
    // Pass the thread handle (t1) to another function to join it
    joinThread(t1);  // Passing the thread handle as a reference

    return 0;
}
