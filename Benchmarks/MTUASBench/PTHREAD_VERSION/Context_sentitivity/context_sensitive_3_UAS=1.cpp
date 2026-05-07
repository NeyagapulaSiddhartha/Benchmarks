#include "/home/sidda/FINDUS_Artifact/FINDUS/Benchmarks/MTUASBench/Utilites/doWork.h"
/* Description: The function UseOfData is called at 2 different locations in spawnThreads function. Here the join 
is placed immediately after the thread spawn so there is no Use After Scope error */

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

void spawnThreads()
{
    int x;
        x=35;

    // cin >> x;
    
    if(x<30)
    {
        // Create thread t1 and join it immediately to avoid Use After Scop
        if(x>10){
            pthread_create(&t1, nullptr, UseOfData, &x);
            pthread_join(t1, nullptr);
        }

    }

        
    if(x>30)
    {
        if(x<40){
            x = 100;
        pthread_create(&t2, nullptr, UseOfData, &x);
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

int main()
{
    spawnThreads();
    pthread_join(t2, nullptr);
    return 0;
}

