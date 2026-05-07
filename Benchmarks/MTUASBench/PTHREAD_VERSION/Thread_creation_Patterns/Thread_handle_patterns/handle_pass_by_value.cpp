#include "/home/sidda/FINDUS_Artifact/FINDUS/Benchmarks/MTUASBench/Utilites/doWork.h"
/* Description: Thread handle is passed by value to a function startThread which uses the same handle
to create thread running threadFunction */

#include <iostream>
#include <pthread.h>

void* threadFunction(void* arg) {
    int counter = *static_cast<int*>(arg);  // Convert void pointer to int pointer
    std::cout << "Thread is running (handle passed by value), counter = " << counter << "\n";
    return nullptr;
}

void startThread(pthread_t t, int counter) {
    pthread_create(&t, nullptr, threadFunction, &counter);  // Create a new thread
    pthread_join(t, nullptr);  // Wait for the thread to finish
}

int main() {
    pthread_t t;  // Thread handle
    int counter = 10;
    startThread(t, counter);  // Pass thread handle by value
    std::cout << "Main function ends\n";
    return 0;
}
