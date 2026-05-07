#include "/home/sidda/FINDUS_Artifact/FINDUS/Benchmarks/MTUASBench/Utilites/doWork.h"
/* Description: spawnThreads function is called at two different locations in the main function.
Depending on the value of x, UseOfData function prints the value of data passed as a reference */

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

void spawnThreads(int* x)
{
    if (*x == 10)
    {
        // Create thread t1 and join it immediately
        pthread_create(&t1, nullptr, UseOfData, x);
        pthread_join(t1, nullptr);
    }
    else
    {
        *x = 100;
        // Create thread t2 and join it immediately
        pthread_create(&t2, nullptr, UseOfData, x);
        pthread_join(t2, nullptr);
    }
}

int main()
{
    int x;
    // cin >> x;
    x=90;

    // Call spawnThreads with initial value of x
    spawnThreads(&x);

    // Modify x and call spawnThreads again
    for (x = 0; x < 10; x++);
    spawnThreads(&x);
    // pthread_join(t1, nullptr);
    // pthread_join(t2, nullptr);
    
    return 0;
}
