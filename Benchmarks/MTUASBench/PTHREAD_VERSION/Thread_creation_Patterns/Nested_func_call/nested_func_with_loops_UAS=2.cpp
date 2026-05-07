#include "/home/sidda/FINDUS_Artifact/FINDUS/Benchmarks/MTUASBench/Utilites/doWork.h"
#include <iostream>
#include <pthread.h>
#include <vector>
#include<unistd.h>
using namespace std;

vector<pthread_t> td;

void* runThread(void* arg){


            for(int j=0;j<5;j++){

                  for(int jj=0;jj<5;jj++){
                     int *y = static_cast<int*>(arg);
                     // cout<<"Mem loc:"<<y<<endl;
                         std::cout << "Value of y is:" << *y << "\n";
                        std::cout<<"doing some work in threads "<<"\n";
                     doWork(1000);
                  }
            }
    return nullptr;
}

void* runThreadByVal(void* arg)
{
                for(int j=0;j<5;j++){

                  for(int jj=0;jj<5;jj++){
                         int t = *static_cast<int*>(arg);
                        cout << "Value passed from func by Val: " << t << endl;
                     doWork(1000);
                  }
            }

    return nullptr;
}

void c(int* z){

    for (int i = 1; i < 7; i++)
    {
        if (i == 1 || i == 2) // Corrected condition
        {
            pthread_t t;
            pthread_create(&t, nullptr, runThread, static_cast<void*>(z)); // Pass pointer
            td.push_back(t); // Store thread
        }
        else
        {
            pthread_t t;
            pthread_create(&t, nullptr, runThreadByVal, static_cast<void*>(z)); // Pass value
            td.push_back(t); // Store thread
        }
            // sleep(10);
    }

}

void b()
{
    int y=30 ;

    if(y >= 10)
    {
         c(&y); // Pass the address of y
    }
   
        // doWork(100);

        for(int i=0;i<7;i++)
        {
            for(int j=0;j<7;j++)
            {
                std::cout<<" doing some work in c "<<"\n";
            }
        }
  
}

void a()
{
    b();
    for (auto& it : td) // Use reference to join threads
    {
        pthread_join(it, nullptr);
    }
}

int main()
{
    a();
    return 0;
}
