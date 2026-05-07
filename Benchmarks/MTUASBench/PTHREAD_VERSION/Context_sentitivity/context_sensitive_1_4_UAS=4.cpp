#include "/home/sidda/FINDUS_Artifact/FINDUS/Benchmarks/MTUASBench/Utilites/doWork.h"
/* Description: Two threads are spawned t1 and t2 in spawnThreads function depending on value of x.
One thread is joined in if block preventing Use After Scope bug whereas the other thread is joined inside
the main function leading to potential Use After Scope. For bug to occur input any value other than 10 */

#include <iostream>
#include <pthread.h>
#include<unistd.h>
using namespace std;

pthread_t t1, t2,t3,t4;

void* UseOfData1(void* arg)
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
void* UseOfData2(void* arg)
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

    x=20;

        pthread_create(&t1, nullptr, UseOfData1, &x);
       

        x = 100;
        int y=200;
        // Create thread t2 without joining it inside spawnThreads, leading to potential Use After Scope
        pthread_create(&t2, nullptr, UseOfData2, &x);
     
        pthread_create(&t3, nullptr, UseOfData1, &y);
        pthread_create(&t4, nullptr, UseOfData2, &y);

       for(int i=0;i<7;i++){

           for(int j=0;j<5;j++)
           {
               std::cout<<"doing some work in thread "<<"\n";
               doWork(1000);
           }
                   doWork(200);

       }
        // pthread_join(t2, nullptr); // Commented to mimic original behavior
    // }
}

int main()
{
    spawnThreads();
    // Join thread t2 in the main function, which may lead to Use After Scope

    pthread_join(t1, nullptr);
    pthread_join(t2, nullptr);
    pthread_join(t3, nullptr);
    pthread_join(t4, nullptr);

    return 0;
}
