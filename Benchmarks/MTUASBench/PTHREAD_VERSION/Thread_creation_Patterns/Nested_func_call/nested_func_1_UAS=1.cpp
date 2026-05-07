#include "/home/sidda/FINDUS_Artifact/FINDUS/Benchmarks/MTUASBench/Utilites/doWork.h"
/* Description: In this program a function call chain is created like main->a()->b()->c()->runthread.
   A data object x is passed as reference from one function to another starting from function a(). 
   In function c local variable is passed to final function in the chain runThread where it is Used. 
   Thread t1 is joined in main function instead of c that results in Use After Scope */

#include <iostream>
#include <pthread.h>
using namespace std;

pthread_t t1;

void* runThread(void *arg) {
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

void c(int *z) {
    int r ; 
    // cin>>r;
    if (r == 10) {

                std::cout<<" doing some work in c "<<"\n";
  
    } else {
        pthread_create(&t1, nullptr, runThread, static_cast<void*>(z));

        // t1.join(); // We will join the thread in main instead of here
    }
}

void b(int *y) {
    int f=20;
    // cin>>f;
    if (f == 20) {
        *y = 30;
         c(y);
    }
}

void a() {
    int x = 10;
    b(&x);
    
       for(int i=0;i<5;i++)
       {
           for(int j=0;j<5;j++)
           {
               std::cout<<"doing some work in thread "<<"\n";
               doWork(1000);
           }
            doWork(200);

       }
}

int main() {
    // int x=10;
    a();
    pthread_join(t1, nullptr);
}
