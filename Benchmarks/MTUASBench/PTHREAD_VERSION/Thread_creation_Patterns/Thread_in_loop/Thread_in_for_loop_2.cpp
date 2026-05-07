#include "/home/sidda/FINDUS_Artifact/FINDUS/Benchmarks/MTUASBench/Utilites/doWork.h"
/* Description: Threads are created in for loop in runThreads() function passing data object z as reference and 
joined immediately in same scope block avoiding potential Use After Scope bugs */

#include <iostream>
#include <pthread.h>
#include <vector>
using namespace std;

// Function executed by threads when passing by value
void* runFunc1(void* arg) {
    int x = *static_cast<int*>(arg);
    cout << "value of data received when pass by val: " << x << endl;
    return nullptr;
}

// Function executed by threads when passing by reference
void* runFunc2(void* arg) {
    int* x = static_cast<int*>(arg);
    cout << "value of data received when pass by ref: " << *x << endl;
    return nullptr;
}

int main() {
    int x = 10, y = 20;

    for (int i = 0; i < 6; i++) {
        pthread_t tid;

        if (i >= 0 && i <= 2) {
            int z = 30;
            pthread_create(&tid, nullptr, runFunc1, &z);
            pthread_join(tid, nullptr);
        } else {
            int z = 30;
            pthread_create(&tid, nullptr, runFunc2, &z);
            pthread_join(tid, nullptr);
        }
    }

    return 0;
}
