#include "/home/sidda/FINDUS_Artifact/FINDUS/Benchmarks/MTUASBench/Utilites/doWork.h"
#include <iostream>
#include <pthread.h>
#include <vector>
using namespace std;

vector<pthread_t> td;

void* runThread(void* arg)
{
    int* t = static_cast<int*>(arg);
    cout << "Value passed from func by Ref: " << *t << endl;
    return nullptr;
}

void* runThreadByVal(void* arg)
{
    int t = *static_cast<int*>(arg);
    cout << "Value passed from func by Val: " << t << endl;
    return nullptr;
}

void c(int* z)
{
    for (int i = 1; i < 5; i++)
    {
        if (i == 1 || i == 2) // Corrected condition
        {
            pthread_t t;
            pthread_create(&t, nullptr, runThread, z); // Pass pointer
            td.push_back(t); // Store thread
        }
        else
        {
            pthread_t t;
            pthread_create(&t, nullptr, runThreadByVal, z); // Pass value
            td.push_back(t); // Store thread
        }
    }

    for (auto& it : td) // Use reference to join threads
    {
        pthread_join(it, nullptr);
    }
}

void b()
{
    int y = 10;
    c(&y); // Pass the address of y
}

void a()
{
    b();
}

int main()
{
    a();
    return 0;
}
