#include "/home/sidda/FINDUS_Artifact/FINDUS/Benchmarks/MTUASBench/Utilites/doWork.h"
#include <iostream>
#include <pthread.h>
#include<unistd.h>

pthread_t t1,t2;

void* thread_func(void *arg) {
    int *x = static_cast<int*>(arg);
    std::cout << "Value of x is:" << *x << "\n";
    // std::cout<<"Value of x is:";
    return nullptr;
}
void join_func(pthread_t t)
{
    pthread_join(t,nullptr);
}

void threadFunction() {
    int x = 10;
    int i=0;
    pthread_create(&t1, nullptr, thread_func, &x);
    pthread_create(&t2, nullptr, thread_func, &x);
    if(x==20){
        pthread_join(t1, nullptr);
        if(i==20)
        {
            join_func(t2);
        }
        else
        {
            join_func(t2);

        }

    }
    else{
        
        pthread_join(t1, nullptr);
        if(i==20)
        {
            join_func(t2);
        }
        else
        {
            join_func(t2);

        }
    } 
    // pthread_join(t2, nullptr);
}

int main() {
    //  int x=10;
    // t1=std::thread(thread_func);
    threadFunction();
    pthread_join(t1, nullptr);
}
