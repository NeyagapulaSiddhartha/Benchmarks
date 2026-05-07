#include "/home/sidda/FINDUS_Artifact/FINDUS/Benchmarks/MTUASBench/Utilites/doWork.h"
// /* Description: A single object is passed as reference from one thread to another at multiple levels. A local object is declared in taskLevel0 which is passed
// all the way to taskLevel5 where it is finally used. Joining the thread at main() instead of taskLevel0 who is the owner of the object leads to Use After Scope 
// bug */


#include <iostream>
#include <unistd.h>
#include <pthread.h>

pthread_t t1, t2, t3, t4, t5;

// Level 5 thread: Uses the passed variable
void* taskLevel5(void* arg) {

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

void* taskLevel4(void* arg) {
    int* ref = static_cast<int*>(arg);
    std::cout << "Level 4 thread is running.\n";
    pthread_create(&t5, nullptr, taskLevel5, ref);
    return nullptr;
}

void* taskLevel3(void* arg) {
    int* ref = static_cast<int*>(arg);
    std::cout << "Level 3 thread is running.\n";
    pthread_create(&t4, nullptr, taskLevel4, ref);
    return nullptr;
}

void* taskLevel2(void* arg) {
    int* ref = static_cast<int*>(arg);
    std::cout << "Level 2 thread is running.\n";
    pthread_create(&t3, nullptr, taskLevel3, ref);
    return nullptr;
}

void* taskLevel1(void* arg) {
    int* ref = static_cast<int*>(arg);
    std::cout << "Level 1 thread is running.\n";
    pthread_create(&t2, nullptr, taskLevel2, ref);
    return nullptr;
}

// Level 0 function that owns the stack variable
void* taskLevel0(void* arg) {
    int localVar = 100;  // Stack variable
    std::cout << "Level 0 is running. Initial value: " << localVar << "\n";

    pthread_create(&t1, nullptr, taskLevel1, &localVar);

          for(int i=0;i<5;i++)
       {
           for(int j=0;j<5;j++)
           {
               std::cout<<"doing some work in thread "<<"\n";
               doWork(1000);
           }
            doWork(200);

       }

    std::cout << "Level 0 is returning. Final value: " << localVar << "\n";
    return nullptr;  // Now localVar goes out of scope
}

int main() {
    taskLevel0(nullptr);

    // Give threads time to run *after* variable is out of scope

    // Join all threads — UAS should have happened already
        pthread_join(t1, nullptr);
        pthread_join(t2, nullptr);
        pthread_join(t3, nullptr);
        pthread_join(t4, nullptr);
        pthread_join(t5, nullptr);
  
  

  

    return 0;
}

