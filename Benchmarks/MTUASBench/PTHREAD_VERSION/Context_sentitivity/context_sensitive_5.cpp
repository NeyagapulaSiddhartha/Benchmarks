#include "/home/sidda/FINDUS_Artifact/FINDUS/Benchmarks/MTUASBench/Utilites/doWork.h"
/* Description: The function UseOfData is called at 2 different locations in spawnThreads function by same thread handle t1 and different data x1 and x2. Here the join 
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
    int x1=10,x2=20;
    // cin >> x1;
    // cin >> x2;
    if (x1 == 10)
    {
        // Create thread t1 and join it immediately to avoid Use After Scope
        pthread_create(&t1, nullptr, UseOfData, &x1);
        pthread_join(t1, nullptr);
    }
    else
    {
        x1 = 100;
        // Create thread t2 and join it immediately to avoid Use After Scope
        pthread_create(&t1, nullptr, UseOfData, &x2);
        pthread_join(t1, nullptr);
    }
}

int main()
{
    spawnThreads();
    return 0;
}
