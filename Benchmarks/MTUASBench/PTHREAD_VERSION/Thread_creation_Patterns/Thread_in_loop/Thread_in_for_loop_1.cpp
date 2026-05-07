#include "/home/sidda/FINDUS_Artifact/FINDUS/Benchmarks/MTUASBench/Utilites/doWork.h"
/* Description: Threads are created inside a for loop and parameter passed as reference. 
The threads are joined inside main() function avoiding potential Use After Scope bugs */

#include <iostream>
#include <pthread.h>
#include <vector>

// Function to be executed by each thread
void* threadTask(void* args) {
    int* ref = static_cast<int*>(args); // Dereference the pointer
    (*ref)++; // Increment the shared variable
    std::cout << "Thread is running. Modified value: " << *ref << "\n";
    return nullptr;
}

int main() {
    int sharedVar = 56;  // A variable shared by all threads

    std::vector<pthread_t> threads;  // Vector to hold thread IDs

    // Create 5 threads in a for loop
    for (int i = 0; i < 5; ++i) {
        pthread_t tid;
        pthread_create(&tid, nullptr, threadTask, &sharedVar);  // Create a new thread, passing sharedVar as reference
        threads.push_back(tid); // Add thread ID to the vector
    }

    // Wait for all threads to finish
    for (auto& t : threads) {
        pthread_join(t, nullptr);  // Join each thread
    }

    std::cout << "Main thread: Final value of sharedVar: " << sharedVar << "\n";

    return 0;
}
