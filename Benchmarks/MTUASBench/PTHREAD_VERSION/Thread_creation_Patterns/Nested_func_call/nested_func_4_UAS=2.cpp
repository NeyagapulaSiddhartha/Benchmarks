#include "/home/sidda/FINDUS_Artifact/FINDUS/Benchmarks/MTUASBench/Utilites/doWork.h"
/* Description: In this program a function call chain is created like main->a()->b()->c()->runThread.
   A data object x is passed as reference from one function to another starting from function b(). 
   In function c local variable is passed to final function in the chain runThread where it is Used. 
   Thread t1 is joined in a() function instead of c that results in  Use After Scope */

#include <iostream>
#include <pthread.h>
#include<unistd.h>
using namespace std;

pthread_t t1,t2;

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

void c() {
    int r = 10;
    // cin>>r;
    if (r < 6) {
        // Do nothing
    } else {
        pthread_create(&t1, nullptr, runThread, &r);
        pthread_create(&t2, nullptr, runThread, &r);
        // Do not join here to avoid Use After Scope
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
   // r=10;
//    cout<<"c  gone"<<endl;
}

void b() {
    int y = 10 ;

    // cin>>y;

    if(y >= 10)
    {
        c();
        pthread_join(t1, nullptr); // Join the thread here to avoid Use After Scope
    }
     for(int i=0;i<10;i++)
        {
            for(int j=0;j<10;j++)
            {
                std::cout<<" doing some work in b "<<"\n";
            }
        }
   
}

void a() {
    b();
    pthread_join(t2,nullptr); // Join the thread here to avoid Use After Scope
   
    
}

int main() {
    a();
   
    // pthread_join(t1, nullptr); // Join the thread here to avoid Use After Scope
}
