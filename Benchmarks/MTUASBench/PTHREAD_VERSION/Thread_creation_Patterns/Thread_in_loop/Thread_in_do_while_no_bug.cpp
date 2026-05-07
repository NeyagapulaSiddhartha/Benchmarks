#include "/home/sidda/FINDUS_Artifact/FINDUS/Benchmarks/MTUASBench/Utilites/doWork.h"
/* Description: 50 threads are created in do while loop. First 40 threads are created in else block and last 10 in if block. The threads are joined outside the 
if-else block leading to Use After Scope bug */

#include <iostream>
#include <pthread.h>
#include <vector>
#include <unistd.h>

using namespace std;

void* threadTask(void* arg) {
    int* data = static_cast<int*>(arg);
    std::cout << endl << "Thread updated sharedData to " << *data << "\n";
    return nullptr;
}

void* threadTask2(void* arg) {
    int data = *static_cast<int*>(arg);
    std::cout << endl << "Thread updated sharedData to " << data << "\n";
    return nullptr;
}

int main() {
    std::vector<pthread_t> threads; // Vector to store thread handles
    int id = 1;
    int tcount = 0;

    // Create thread chain with data dependency
    do {
        pthread_t tid; // Temporary thread handle
        if (tcount > 40) {
            int sharedData = 2;
            pthread_create(&tid, nullptr, threadTask2, &sharedData);
            threads.push_back(tid);
            // tcount++;
        } else {
            int sharedData = 2;
            pthread_create(&tid, nullptr, threadTask, &sharedData);
            threads.push_back(tid);
            // tcount++;
        }
        tcount++;
    } while (tcount < 50);

        doWork(1000); 

    cout << "Loop completed !!!" << endl;

    for (int i = 0; i < tcount; i++) {
        std::cout << "Thread " << i + 1 << " is joining" << endl;
        pthread_join(threads[i], nullptr); // Join all threads
    }

    return 0;
}
