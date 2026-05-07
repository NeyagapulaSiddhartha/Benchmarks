#include "/home/sidda/FINDUS_Artifact/FINDUS/Benchmarks/MTUASBench/Utilites/doWork.h"
/* Description: A function createThread is called in main with sharedVar passed as ref
which internally creates thread to run threadTask function */

#include <iostream>
#include <pthread.h>  // for pthread
using namespace std;

int a = 10;

void* runGlobalThread(void* arg)
{
    int* a = static_cast<int*>(arg);
    cout << "Value in global thread: " << *a << endl;
    return nullptr;
}

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

pthread_t global_t;

int main()
{
    int x = 10;
    pthread_t local_t;
    pthread_create(&local_t, nullptr, runLocalThread1, &x);

    if (x == 1)
    {
        pthread_t if_thread;
        pthread_create(&if_thread, nullptr, runLocalThread2, &x);
        x = 11;
        pthread_join(if_thread, nullptr);
    }
    else
    {
        pthread_t else_thread;
        pthread_create(&else_thread, nullptr, runLocalThread3, &x);
        x = 12;
        pthread_join(else_thread, nullptr);
    }

    {
        pthread_t scope_1;
        pthread_create(&scope_1, nullptr, runLocalThread4, &x);

        {
            pthread_t scope_2;
            pthread_create(&scope_2, nullptr, runLocalThread5, &x);

            {
                pthread_t scope_3;
                pthread_create(&scope_3, nullptr, runLocalThread6, &x);

                {
                    pthread_t scope_n;
                    pthread_create(&scope_n, nullptr, runLocalThread7, &x);

                    pthread_join(scope_1, nullptr);
                    pthread_join(scope_2, nullptr);
                    pthread_join(scope_3, nullptr);

                    pthread_join(scope_n, nullptr);
                }
            }
        }
    }

    pthread_create(&global_t, nullptr, runGlobalThread, &a);
    pthread_join(local_t, nullptr);
    pthread_join(global_t, nullptr);

    return 0;
}
