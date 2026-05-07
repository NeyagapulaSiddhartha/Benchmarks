#include "/home/sidda/FINDUS_Artifact/FINDUS/Benchmarks/MTUASBench/Utilites/doWork.h"
/* Description: The function UseOfData is called at 2 different locations in spawnThreads function. Here the join 
is placed immediately after the thread spawn so there is no Use After Scope error */

#include <iostream>
#include <pthread.h>
using namespace std;

pthread_t t1, t2;

void* UseOfData(void* arg)
{
    int* x = (int*)arg;
    cout << "Data sent:" << *x << endl;
    return nullptr;
}

void spawnThreads()
{
    int x;
    // cin >> x;
    x=100;
    
    // Create thread t1 and join it immediately to avoid Use After Scope
    pthread_create(&t1, nullptr, UseOfData, &x);
    pthread_join(t1, nullptr);

    x = 100;
    // Create thread t2 and join it immediately to avoid Use After Scope
    pthread_create(&t2, nullptr, UseOfData, &x);
    pthread_join(t2, nullptr);
  
}

int main()
{
    spawnThreads();
    return 0;
}

