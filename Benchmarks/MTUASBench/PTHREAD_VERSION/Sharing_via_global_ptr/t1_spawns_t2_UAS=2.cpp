#include "/home/sidda/FINDUS_Artifact/FINDUS/Benchmarks/MTUASBench/Utilites/doWork.h"
#include <iostream>
#include <pthread.h>
#include <unistd.h> // for 
using namespace std;
int *gPtr1,*gPtr2;
pthread_t t1,t2,t3;
void* t2_func(void* arg) {

            for(int j=0;j<5;j++){

                  for(int jj=0;jj<5;jj++){

                 std::cout<<" this the work from thread 2 "<<*gPtr1<<*gPtr2<<"\n";
                            doWork(1000);
                  }
                              doWork(200);

            }
    return nullptr;
}

void* t1_func(void* arg) {

    int local1 = 30;
    int local2 = 40;
    gPtr1 = &local1;
    gPtr2 = &local2;
    pthread_create(&t2, NULL, t2_func, NULL);
    
    pthread_create(&t3, NULL, t2_func, NULL);

       for(int i=0;i<5;i++)
       {
           for(int j=0;j<5;j++)
           {
               std::cout<<"doing some work in thread "<<"\n";
               doWork(1000);
           }
            doWork(200);

       }
    // pthread_join(t2, NULL); // T2 => T1
        return nullptr;

}

int main() {
    pthread_create(&t1, NULL, t1_func, NULL);
    pthread_join(t1, NULL);
        pthread_join(t2, NULL); // T2 => T1

            pthread_join(t3, NULL); // T2 => T1

    return 0;
}
