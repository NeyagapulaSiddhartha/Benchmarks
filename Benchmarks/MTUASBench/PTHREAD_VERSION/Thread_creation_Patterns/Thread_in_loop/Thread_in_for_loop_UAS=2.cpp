#include "/home/sidda/FINDUS_Artifact/FINDUS/Benchmarks/MTUASBench/Utilites/doWork.h"
/* Description: Threads are created in for loop in runThreads() function passing data object z as reference 
and joined in main() function giving rise to Use After Scope bugs */

#include <iostream>
#include <pthread.h>
#include <vector>
using namespace std;

// Function for threads handling pass-by-value
void* runFunc1(void* args) {
    int x = *(static_cast<int*>(args)); // Dereference the value
    cout << "Value of data received when passed by val: " << x << endl;
    return nullptr;
}

// Function for threads handling pass-by-reference
void* runFunc2(void* args) {
    int* x = static_cast<int*>(args); // Use reference
    cout << "Value of data received when passed by ref: " << *x << endl;
    return nullptr;
}

// Global vector to hold thread IDs
std::vector<pthread_t> threads;

void runThreads() {
    int x = 10, y = 20;

    for (int i = 0; i < 4; i++) {
        pthread_t tid;
        if (i >= 0 && i <= 2) {
            int z = 30; // Value to pass
            pthread_create(&tid, nullptr, runFunc1, &z);
        } else {
            int z = 30; // Reference to pass
            pthread_create(&tid, nullptr, runFunc2, &z);
        }
        threads.push_back(tid); // Add thread ID to the vector
    }
}

int main() {
    runThreads();

    // Wait for all threads to finish
    for (auto& thread : threads) {
        pthread_join(thread, nullptr);
    }

    return 0;
}
