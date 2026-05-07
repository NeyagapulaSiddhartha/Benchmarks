#include "/home/sidda/FINDUS_Artifact/FINDUS/Benchmarks/MTUASBench/Utilites/doWork.h"
/* Description: In this program a function call chain is created like main->a()->b()->c()->runThread.
   A data object x is passed as reference from one function to another starting from function a(). 
   In function c local variable is passed to final function in the chain runThread where it is Used. 
   Thread t1 is joined in main function instead of c that results in  Use After Scope */

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
    int r = 1;
    if (r == 10) {
        // Do nothing
    } else {
        pthread_create(&t1, nullptr, runThread, static_cast<void*>(z));
        // pthread_join(t1, nullptr); // Thread join should be done inside c to avoid Use After Scope
    }
}

void b(int *y) {
    int f;
    cin>>f;
    if (f == 20) {
        *y = 30;
         c(y);
    }
    pthread_create(&t2, nullptr, runThread, static_cast<void*>(y));
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
    a();
    pthread_join(t1, nullptr); // Join the thread in main
}
