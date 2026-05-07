#include "/home/sidda/FINDUS_Artifact/FINDUS/Benchmarks/MTUASBench/Utilites/doWork.h"
#include <iostream>
#include <pthread.h>
#include<unistd.h>

pthread_t t1,t2;

void* thread_func(void *arg) {
            for(int j=0;j<5;j++){

                  for(int jj=0;jj<5;jj++){
                     int *y = static_cast<int*>(arg);
                     // cout<<"Mem loc:"<<y<<endl;
                         std::cout << "Value of y is:" << *y << "\n";
                        std::cout<<"doing some work in main "<<"\n";
                     doWork(1000);
                  }
            }
    return nullptr;
}
void join_func(pthread_t t)
{
    pthread_join(t,nullptr);
}

void threadFunction() {
    int x = 10;
    // cin>>x;
    pthread_create(&t1, nullptr, thread_func, &x);
    pthread_create(&t2, nullptr, thread_func, &x);
    if(x>=0){

        pthread_join(t1, nullptr);

    }
    else 
    {
        pthread_join(t1, nullptr);

    }
    join_func(t2);
    
    pthread_join(t2, nullptr);
}

int main() {
    //  int x=10;
    // t1=std::thread(thread_func);
    threadFunction();
    // pthread_join(t1, nullptr);
}
