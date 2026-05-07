#include "/home/sidda/FINDUS_Artifact/FINDUS/Benchmarks/MTUASBench/Utilites/doWork.h"
/* Description: Object x is created on stack (local var) and then passed as reference to thread function thread_func. Here the thread t1 is joined immediately
   after it is spawned inside threadFunction which extends the life of threadFunction and there is no Use After Scope bug */

#include <iostream>
#include <pthread.h>

pthread_t t1;

void* thread_func(void *arg) {
    int *x = static_cast<int*>(arg);
    std::cout << "Value of x is:" << *x << "\n";
    // std::cout<<"Value of x is:";
    return nullptr;
}

void threadFunction() {
    int x = 10;
    pthread_create(&t1, nullptr, thread_func, static_cast<void*>(&x));
    pthread_join(t1, nullptr);
}

int main() {
    //  int x=10;
    // t1=std::thread(thread_func);
    threadFunction();
}
