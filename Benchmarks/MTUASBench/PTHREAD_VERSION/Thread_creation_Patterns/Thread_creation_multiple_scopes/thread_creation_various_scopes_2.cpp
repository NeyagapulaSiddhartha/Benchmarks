#include "/home/sidda/FINDUS_Artifact/FINDUS/Benchmarks/MTUASBench/Utilites/doWork.h"
/* Description: A function createThread is called in main with sharedVar passed as ref
which internally creates thread to run threadTask function */

#include <iostream>
#include <pthread.h>  // for pthread
#include <functional>  // for std::ref
using namespace std;

void* runLocalThread1(void* arg)
{
    int* x = static_cast<int*>(arg);
    cout << "Value in local thread 1: " << *x << endl;
    return nullptr;
}

void* runLocalThread2(void* arg)
{
    int* x = static_cast<int*>(arg);
    cout << "Value in local thread 2: " << *x << endl;
    return nullptr;
}

void* runLocalThread3(void* arg)
{
    int* x = static_cast<int*>(arg);
    cout << "Value in local thread 3: " << *x << endl;
    return nullptr;
}

void* runLocalThread4(void* arg)
{
    int* x = static_cast<int*>(arg);
    cout << "Value in local thread 4: " << *x << endl;
    return nullptr;
}

void* runLocalThread5(void* arg)
{
    int* x = static_cast<int*>(arg);
    cout << "Value in local thread 5: " << *x << endl;
    return nullptr;
}

void* runLocalThread6(void* arg)
{
    int* x = static_cast<int*>(arg);
    cout << "Value in local thread 6: " << *x << endl;
    return nullptr;
}

void* runLocalThread7(void* arg)
{
    int* x = static_cast<int*>(arg);
    cout << "Value in local thread 7: " << *x << endl;
    return nullptr;
}

pthread_t scope_1;
pthread_t scope_2;
pthread_t scope_3;
pthread_t scope_n;

int main()
{
    {
        int s1_var = 1;
        pthread_create(&scope_1, nullptr, runLocalThread1, &s1_var);

        {
            int s2_var = 2;
            pthread_create(&scope_2, nullptr, runLocalThread2, &s2_var);
            {
                int s3_var = 3;
                pthread_create(&scope_3, nullptr, runLocalThread3, &s3_var);
                {
                    {
                        int sn_var = 4;
                        pthread_create(&scope_n, nullptr, runLocalThread4, &sn_var);
                    }
                }
            }
        }
    }

    pthread_join(scope_1, nullptr);
    pthread_join(scope_2, nullptr);
    pthread_join(scope_3, nullptr);
    pthread_join(scope_n, nullptr);

    return 0;
}
