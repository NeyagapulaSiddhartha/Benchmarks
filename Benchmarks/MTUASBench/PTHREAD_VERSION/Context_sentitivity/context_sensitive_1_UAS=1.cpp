#include "/home/sidda/FINDUS_Artifact/FINDUS/Benchmarks/MTUASBench/Utilites/doWork.h"
/* Description: Two threads are spawned t1 and t2 in spawnThreads function depending on value of x.
One thread is joined in if block preventing Use After Scope bug whereas the other thread is joined inside
the main function leading to potential Use After Scope. For bug to occur input any value other than 10 */

#include <iostream>
#include <pthread.h>
#include <unistd.h>
using namespace std;

pthread_t t1, t2;

void* UseOfData(void* arg)
{
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

void spawnThreads(){
    int x=-9;
    // cin >> x;
    if (x >= 10){
        // Create thread t1 and join it inside the if block to avoid Use After Scope
        pthread_create(&t1, nullptr, UseOfData, &x);
                pthread_join(t1, nullptr);

    }
    else{
        x = 100;
        // Create thread t2 without joining it inside spawnThreads, leading to potential Use After Scope
        pthread_create(&t2, nullptr, UseOfData, &x);
    
    
    for(int i=0;i<50;i++){

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

    }
}

int main(){
    spawnThreads();
    // Join thread t2 in the main function, which may lead to Use After Scope
    pthread_join(t2, nullptr);
    return 0;
}
