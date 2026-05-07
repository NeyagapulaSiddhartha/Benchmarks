#include "/home/sidda/FINDUS_Artifact/FINDUS/Benchmarks/MTUASBench/Utilites/doWork.h"

#include <iostream>
#include <pthread.h>
#include <unistd.h> // for 
using namespace std;
int *gPtr;
// int check=0;
pthread_t t1,t2;
void* t1_func(void* arg) {
    int local = 10;
    gPtr = &local;
    // check=1;
    printf("T1 assigned: %d\n", local);
   for( int i =0; i<100; i++)
   {
    std::cout<<" perform some work while waiting for updated info "<<"\n";
   }
   printf("T2 read: %d\n", *gPtr);
    return NULL;
}

void* t2_func(void* arg) {
    // while(!check);
   
              for(int j=0;j<5;j++){

                  for(int jj=0;jj<5;jj++){
                     int *y = static_cast<int*>(arg);
                     // cout<<"Mem loc:"<<y<<endl;
                         std::cout << "Value of y is:" << (*gPtr)++; << "\n";
                        std::cout<<"doing some work in main "<<"\n";
                     doWork(1000);
                  }
                              doWork(200);
            }
    return nullptr;
}

int main() {
    pthread_create(&t1, NULL, t1_func, NULL);
    while(gPtr==nullptr);
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
