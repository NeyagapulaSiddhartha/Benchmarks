#include "/home/sidda/FINDUS_Artifact/FINDUS/Benchmarks/MTUASBench/Utilites/doWork.h"
/* Description: Thread handle created on heap using new keyword in main function. 
Same handle used to run threadFunction passing counter as reference */

#include <iostream>
#include <pthread.h>

void* threadFunction(void* arg) {
    int* counter = static_cast<int*>(arg);
    std::cout << "Thread is running, counter = " << *counter << "\n";
    return nullptr;
}

int main() {
    int counter = 5;

    pthread_t* t = new pthread_t;  // Create a new thread handle

    // Create a new thread
    pthread_create(t, nullptr, threadFunction, &counter);

    // Ensure the thread is joined before proceeding
    if (pthread_join(*t, nullptr) != 0) {
        std::cerr << "Error joining thread\n";
    }

    delete t;  // Clean up dynamically allocated thread handle

    std::cout << "Main function ends\n";
    return 0;
}
