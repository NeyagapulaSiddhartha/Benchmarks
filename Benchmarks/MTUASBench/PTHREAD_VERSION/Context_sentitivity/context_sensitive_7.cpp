#include "/home/sidda/FINDUS_Artifact/FINDUS/Benchmarks/MTUASBench/Utilites/doWork.h"
/* Description: The function UseOfData is called at 2 different locations in spawnThreads function by  thread handle t1 ,
differnet functions and different datas x1 and x2. Here the join 
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
    int x1=100;
    int x2=200;
    // cin >> x1;
    // cin>>x2;
    if (x1 == 10)
    {
        // Create thread t1 and join it immediately to avoid Use After Scope
        pthread_create(&t1, nullptr, UseOfData1, &x1);
        pthread_join(t1, nullptr);
    }
    else
    {
        x1 = 100;
        // Create thread t2 and join it immediately to avoid Use After Scope
        pthread_create(&t2, nullptr, UseOfData2, &x2);
        pthread_join(t2, nullptr);
    }
}

int main()
{
    spawnThreads();
    return 0;
}
