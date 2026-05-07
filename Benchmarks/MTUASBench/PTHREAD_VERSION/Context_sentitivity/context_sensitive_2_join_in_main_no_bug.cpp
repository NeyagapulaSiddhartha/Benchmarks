#include "/home/sidda/FINDUS_Artifact/FINDUS/Benchmarks/MTUASBench/Utilites/doWork.h"
/* Description: spawnThreads function is called at two different locations in the main function.
Depending on the value of x, UseOfData function prints the value of data passed as a reference */

#include <iostream>
#include <pthread.h>
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

void spawnThreads(int* x)
{
    if (*x == 10)
    {
        // Create thread t1 and join it immediately
        pthread_create(&t1, nullptr, UseOfData, x);
        
    }
    else{
        *x = 100;
        // Create thread t2 and join it immediately
        pthread_create(&t2, nullptr, UseOfData, x);
       
    }
}

int main(){
    int x;
    // cin >> x;
    x=90;

    // Call spawnThreads with initial value of x
    spawnThreads(&x);

    // Modify x and call spawnThreads again

    spawnThreads(&x);
    pthread_join(t1, nullptr);
    pthread_join(t2, nullptr);

    return 0;
}
