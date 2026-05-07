#include "/home/sidda/FINDUS_Artifact/FINDUS/Benchmarks/MTUASBench/Utilites/doWork.h"
/* Description: The function UseOfData is called at 2 different locations in spawnThreads function by  thread handle t1 ,
differnet functions but  same data x. Here the join 
is placed immediately after the thread spawn so there is no Use After Scope error */

#include <iostream>
#include <pthread.h>
using namespace std;

pthread_t t1, t2;

void* UseOfData1(void* arg)
{
    int* x = (int*)arg;
    cout << "Data sent:" << *x << endl;
    return nullptr;
}
void* UseOfData2(void* arg)
{
    int* x = (int*)arg;
    cout << "Data sent:" << *x << endl;
    return nullptr;
}

void spawnThreads()
{
    int x;
    // cin >> x;
    x=30;
    if (x == 10)
    {
        // Create thread t1 and join it immediately to avoid Use After Scope
        pthread_create(&t1, nullptr, UseOfData1, &x);
        pthread_join(t1, nullptr);
    }
    else
    {
        x = 100;
        // Create thread t2 and join it immediately to avoid Use After Scope
        pthread_create(&t2, nullptr, UseOfData2, &x);
        pthread_join(t2, nullptr);
    }
}

int main()
{
    spawnThreads();
    return 0;
}
