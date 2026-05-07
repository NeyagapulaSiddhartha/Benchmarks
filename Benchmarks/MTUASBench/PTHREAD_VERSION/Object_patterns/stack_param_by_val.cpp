#include "/home/sidda/FINDUS_Artifact/FINDUS/Benchmarks/MTUASBench/Utilites/doWork.h"
/* Description: Object x is created on stack (local var) and then passed as value to thread function thread_func. Since parameter is passed by value there is 
   no Use After Scope bug */

#include <iostream>
#include <pthread.h>

pthread_t t1;

void* thread_func(void *arg) {
    int x = *static_cast<int*>(arg);
    std::cout << "Value of x is:" << x << "\n";
    return nullptr;
}

void threadFunction() {
    int x = 10;//stack_param_by_val
    pthread_create(&t1, nullptr, thread_func, static_cast<void*>(&x));
    //  t1.join();
}

int main() {
    threadFunction();
    pthread_join(t1, nullptr);
}
