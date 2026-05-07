#include "/home/sidda/FINDUS_Artifact/FINDUS/Benchmarks/MTUASBench/Utilites/doWork.h"
/* Description: Thread handle is initialized globally. Thread is created in function startThread
running thread function where local value is passed by reference. This is not a buggy program as the thread is
joined in startThread function itself where it is created */

#include <iostream>
#include <pthread.h>

// Global thread handle
pthread_t t;

// Thread function
void* threadFunction(void* arg) {
    int* counter = static_cast<int*>(arg);
    ++(*counter);  // Increment the counter
    std::cout << "Thread is running (global handle), counter = " << *counter << "\n";
    return nullptr;
}

// Function where the thread is created
void startThread() {
    int counter = 0;
    pthread_create(&t, nullptr, threadFunction, &counter);  // Pass counter by pointer
    pthread_join(t, nullptr);  // Ensure the thread is joined before the function returns
}

int main() {
    startThread();
    std::cout << "Main function ends" << "\n";  
    return 0;
}
