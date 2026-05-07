#include "/home/sidda/FINDUS_Artifact/FINDUS/Benchmarks/MTUASBench/Utilites/doWork.h"




#include <iostream>
#include <pthread.h>
#include <unistd.h> // for 
using namespace std;
int *gPtr;
pthread_t t1,t2;

void* t2_func(void* arg) {
            for(int j=0;j<5;j++){

                  for(int jj=0;jj<5;jj++){
                     // cout<<"Mem loc:"<<y<<endl;
                         std::cout << "Value of y is:" << *gPtr << "\n";
                     doWork(1000);
                  }
                              doWork(200);

            }
    return nullptr;
}

void* t1_func(void* arg) {
    int local = 30;
    gPtr = &local;
    pthread_create(&t2, NULL, t2_func, NULL);
    for(int i=0;i<5;i++)
       {
           for(int j=0;j<5;j++)
           {
               std::cout<<"doing some work in thread "<<"\n";
               doWork(1000);
           }
            doWork(200);

       }
    return NULL;
}

int main() {
    pthread_create(&t1, NULL, t1_func, NULL);
    pthread_join(t1, NULL);
        pthread_join(t2, NULL);

    return 0;
}
