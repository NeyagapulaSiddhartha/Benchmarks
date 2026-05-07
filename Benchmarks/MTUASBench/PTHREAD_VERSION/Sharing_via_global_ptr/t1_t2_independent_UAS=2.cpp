#include "/home/sidda/FINDUS_Artifact/FINDUS/Benchmarks/MTUASBench/Utilites/doWork.h"

#include <iostream>
#include <pthread.h>
#include <unistd.h> // for 
using namespace std;
int *gPtr1,*gPtr2;
// int check=0;
pthread_t t1,t2;
void* t1_func(void* arg) {
                        int local1 = 10;
                        int local2 = 20;
                        gPtr1 = &local1;
                        gPtr2 = &local2;
    printf("T1 assigned: %d\n", local1);
    printf("T1 assigned: %d\n", local2);
    printf("T1 assigned: %d\n", local1);
    printf("T1 assigned: %d\n", local2);


    return nullptr;
}

void* t2_func(void* arg) {

              for(int j=0;j<5;j++){

                  for(int jj=0;jj<5;jj++){
                           printf("T2 read: %d\n", *gPtr1);
                        printf("T2 read: %d\n", *gPtr2);
                    // check=1;
                                    doWork(1000);
                  }
                              doWork(200);

            }
    return nullptr;
}

int main() {
    gPtr2=nullptr;
        gPtr1=nullptr;

    pthread_create(&t1, NULL, t1_func, NULL);
    while(gPtr2==nullptr);
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
    pthread_join(t1, NULL); // T2 => T1
    pthread_join(t2, NULL);
    
    return 0;
}
