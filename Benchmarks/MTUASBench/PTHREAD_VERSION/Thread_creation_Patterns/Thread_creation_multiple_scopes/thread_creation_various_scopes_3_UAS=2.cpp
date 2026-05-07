#include "/home/sidda/FINDUS_Artifact/FINDUS/Benchmarks/MTUASBench/Utilites/doWork.h"
/* Description: A function createThread is called in main with sharedVar passed as ref
which internally creates thread to run threadTask function */

#include <iostream>
#include <pthread.h>  // for pthread
#include <functional>  // for std::ref
#include<unistd.h>
using namespace std;

void* runLocalThread1(void* arg)
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

void* runLocalThread2(void* arg)
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

void* runLocalThread3(void* arg)
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

void* runLocalThread4(void* arg)
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

void* runLocalThread5(void* arg)
{
    int* x = static_cast<int*>(arg);
    cout << "Value in local thread 5: " << *x << endl;
    return nullptr;
}

void* runLocalThread6(void* arg)
{
    int* x = static_cast<int*>(arg);
    cout << "Value in local thread 6: " << *x << endl;
    return nullptr;
}

void* runLocalThread7(void* arg)
{
    int* x = static_cast<int*>(arg);
    cout << "Value in local thread 7: " << *x << endl;
    return nullptr;
}

pthread_t scope_1;
pthread_t scope_2;
pthread_t scope_3;
pthread_t scope_n;

int main()
{
    int i=10;
    if(i==10){
        
        int s1_var = 1;
        pthread_create(&scope_1, nullptr, runLocalThread1, &s1_var);

        if(i==10){

            int s2_var = 2;
           
            if(i==10)
            {
                int s3_var = 3;
               
                if(i==10)
                {
                        int sn_var = 4;
                        pthread_create(&scope_n, nullptr, runLocalThread4, &sn_var);

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
    }
    cout<<"loop scope ended"<<endl;

    pthread_join(scope_1, nullptr);
    pthread_join(scope_2, nullptr);
    pthread_join(scope_3, nullptr);
    pthread_join(scope_n, nullptr);

    return 0;
}
