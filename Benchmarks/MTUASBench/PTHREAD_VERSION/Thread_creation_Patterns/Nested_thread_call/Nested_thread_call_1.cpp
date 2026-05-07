#include "/home/sidda/FINDUS_Artifact/FINDUS/Benchmarks/MTUASBench/Utilites/doWork.h"
/* description: A single object is passed as reference from one thread to another at multiple levels. Data object local_var is declared in main function
and passed all the way to thread taskLevel5 where it is used. Thread join is placed in main fucntion which avoids potential Use After Scope */

#include <iostream>
#include <pthread.h>
#include <functional> // for std::ref

// Function for level 5 thread
void* taskLevel5(void* arg) {
    int* ref = static_cast<int*>(arg);
    *ref += 50;  // Modify the variable by reference
    std::cout << "Level 5 thread is running. Modified value: " << *ref << "\n";
    return nullptr;
}

// Function for level 4 thread
void* taskLevel4(void* arg) {
    int* ref = static_cast<int*>(arg);
    std::cout << "Level 4 thread is running.\n";
    pthread_t t5;
    pthread_create(&t5, nullptr, taskLevel5, ref);  // Pass reference to level 5
    pthread_join(t5, nullptr);  // Wait for level 5 to finish
    std::cout << "Level 4 thread finished after level 5. Current value: " << *ref << "\n";
    return nullptr;
}

// Function for level 3 thread
void* taskLevel3(void* arg) {
    int* ref = static_cast<int*>(arg);
    std::cout << "Level 3 thread is running.\n";
    pthread_t t4;
    pthread_create(&t4, nullptr, taskLevel4, ref);  // Pass reference to level 4
    pthread_join(t4, nullptr);  // Wait for level 4 to finish
    std::cout << "Level 3 thread finished after level 4. Current value: " << *ref << "\n";
    return nullptr;
}

// Function for level 2 thread
void* taskLevel2(void* arg) {
    int* ref = static_cast<int*>(arg);
    std::cout << "Level 2 thread is running.\n";
    pthread_t t3;
    pthread_create(&t3, nullptr, taskLevel3, ref);  // Pass reference to level 3
    pthread_join(t3, nullptr);  // Wait for level 3 to finish
    std::cout << "Level 2 thread finished after level 3. Current value: " << *ref << "\n";
    return nullptr;
}

// Function for level 1 thread
void* taskLevel1(void* arg) {
    int* ref = static_cast<int*>(arg);
    std::cout << "Level 1 thread is running.\n";
    pthread_t t2;
    pthread_create(&t2, nullptr, taskLevel2, ref);  // Pass reference to level 2
    pthread_join(t2, nullptr);  // Wait for level 2 to finish
    std::cout << "Level 1 thread finished after level 2. Current value: " << *ref << "\n";
    return nullptr;
}

// Main function
int main() {
    int localVar = 100;  // Local variable to be passed by reference
    std::cout << "Main thread is running. Initial value: " << localVar << "\n";
    
    pthread_t t1;
    pthread_create(&t1, nullptr, taskLevel1, &localVar);
    pthread_join(t1, nullptr);  // Wait for level 1 thread to finish
    
    std::cout << "Main thread finished after level 1. Final value: " << localVar << "\n";
    return 0;
}
