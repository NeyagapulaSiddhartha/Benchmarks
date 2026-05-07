#include "/home/sidda/FINDUS_Artifact/FINDUS/Benchmarks/MTUASBench/Utilites/doWork.h"
/* Description: Thread creation inside a function. A local variable by name localVar
   passed as a reference to the task function. Thread is joined in the same createThread function so there is no bug */

#include <iostream>
#include <pthread.h>

void* task(void* arg) {
    int* ref = static_cast<int*>(arg);  // Cast arg to an int pointer
    *ref += 10;
    std::cout << "Task running in thread, modified value: " << *ref << "\n";
    return nullptr;
}

void createThread() {
    int localVar = 5;
    
    std::cout << "Before thread, localVar: " << localVar << "\n";
    
    pthread_t t;
    pthread_create(&t, nullptr, task, &localVar);  // Create thread and pass the address of localVar
    
    pthread_join(t, nullptr);  // Ensure the thread is joined before continuing
    
    std::cout << "After thread, localVar: " << localVar << "\n";
}

int main() {
    createThread(); 
    return 0;
}
