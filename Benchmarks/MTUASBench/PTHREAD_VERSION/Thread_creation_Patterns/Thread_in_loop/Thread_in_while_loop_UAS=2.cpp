#include "/home/sidda/FINDUS_Artifact/FINDUS/Benchmarks/MTUASBench/Utilites/doWork.h"
/* Description: Threads are created inside while loop and joined immediately in same block avoiding potential Use After Scope bugs */
#include <iostream>
#include <pthread.h>
#include <vector>
using namespace std;

// Thread function
void* func1(void* args) {
    auto* data = static_cast<pair<int*, int>*>(args);
    int* y = data->first;
    int id = data->second;
    *y = *y + 1;
    cout << "Incremented data by id " << id << " and with val " << *y << endl;
    delete data; // Clean up allocated memory
    return nullptr;
}
pthread_t tid;
int spawnThread() {
    int i = 0;

    while (i <= 5) {
        i++;
        if (i == 3) {
            int sharedData =i; // Dynamically allocate memory to avoid scope issues
            
            auto* args = new pair<int*, int>(&sharedData, i + 1); // Pass data and ID
            pthread_create(&tid, nullptr, func1, args);
            // pthread_join(tid, nullptr);
            // delete sharedData; // Clean up allocated memory
        } else {
            int* localData = new int(10); // Dynamically allocate memory to avoid scope issues
            pthread_t tid;
            auto* args = new pair<int*, int>(localData, i + 1); // Pass data and ID
            pthread_create(&tid, nullptr, func1, args);
            // pthread_join(tid, nullptr);
            delete localData; // Clean up allocated memory
        }
    }

    return 0;
}

int main()
{
    spawnThread();
    return 0;

}
