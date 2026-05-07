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
                     doWork(700);
                  }

            }
    return nullptr;
}
void* thread_func2(void *arg) {
            for(int j=0;j<5;j++){

                  for(int jj=0;jj<5;jj++){

                     int *y = static_cast<int*>(arg);
                     // cout<<"Mem loc:"<<y<<endl;
                         std::cout << "Value of y is:" << *y << "\n";
                     doWork(2000);
                  }
                              doWork(200);

            }
    return nullptr;
}

void threadFunction() {
    int x = 10,y=20;
    pthread_create(&t1, nullptr, thread_func, &x);
  



    if(x>=0)
    {
        if(y>=10)
        {
        pthread_create(&t2, nullptr, thread_func2, &x);

         pthread_join(t1, nullptr);

        }
        else{

            pthread_join(t1, nullptr);
        }
    }
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
    
    threadFunction();
    pthread_join(t2, nullptr);
}
