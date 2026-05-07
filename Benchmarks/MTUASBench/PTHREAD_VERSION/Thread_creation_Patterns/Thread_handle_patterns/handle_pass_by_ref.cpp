#include "/home/sidda/FINDUS_Artifact/FINDUS/Benchmarks/MTUASBench/Utilites/doWork.h"
/* Description: Thread handle is passed by reference to a function startThread which uses the same handle
to create thread running threadFunction */

#include <iostream>
#include <pthread.h>

void* threadFunction(void* arg) {
    int counter = *static_cast<int*>(arg);  // Convert void pointer to int pointer
    std::cout << "Thread is running (handle passed by reference), counter = " << counter << "\n";
    return nullptr;
}

void runThread(pthread_t *t, int counter=2) {
    pthread_create(t, nullptr, threadFunction, &counter);  // Create a new thread
    pthread_join(*t, nullptr);  // Wait for the thread to finish
}

// int startThread() {
//     pthread_t t;  // Thread handle
//     int counter = 5;
//     runThread(&t, counter);  // Pass thread handle by reference
//     pthread_join(t, nullptr);  // Wait for the thread to finish
//     std::cout << "Main function ends\n";
//     return 0;
// }

// int main() {
//     startThread();
//     return 0;
// }
