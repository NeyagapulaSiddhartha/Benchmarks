#include "/home/sidda/FINDUS_Artifact/FINDUS/Benchmarks/MTUASBench/Utilites/doWork.h"
/* description: A single object is passed as reference from one thread to another at multiple levels. In taskLevel3 10 threads are spawned. If value of i is 15
the object is passed further to task Level4. Else the threads are spawned running function level3ThreadTask. To avoid use After Scope all threads are joined 
in taskLevel3 in another loop */

#include <iostream>
#include <vector>
#include <unistd.h>
#include <pthread.h>

// Function for level 5 thread
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

// Function for level 4 thread
void* taskLevel4(void* arg) {
    int* ref = static_cast<int*>(arg);
    std::cout << "Level 4 thread is running.\n";
    pthread_t t5;
    pthread_create(&t5, nullptr, taskLevel5, ref);  // Pass reference to level 5
    pthread_join(t5, nullptr);  // Wait for level 5 to finish
    std::cout << "Level 4 thread finished after level 5. Current value: " << *ref << "\n";
    return nullptr;
}

// Helper function for threads in taskLevel3
void* level3ThreadTask(void* arg) {
    int id = *static_cast<int*>(arg);
    std::cout << "Thread in Level 3 running. ID: " << id << "\n";
    return nullptr;
}

// Function for level 3 thread
void* taskLevel3(void* arg) {
    int localVar = 100;  // Local variable to be passed by reference
    std::cout << "Level 3 thread is running.\n";
    
    std::vector<pthread_t> threads; // Vector to store threads
    for (int i = 10; i < 20; ++i) {
        if (i == 15) {
            pthread_t t4;
            pthread_create(&t4, nullptr, taskLevel4, &localVar);  // Pass reference to level 4
            pthread_join(t4, nullptr);  // Join level 4 thread immediately
        } else {
            pthread_t t3Thread;
            pthread_create(&t3Thread, nullptr, level3ThreadTask, &i);  // Pass ID and reference to shared data
            // threads.push_back(t3Thread);  // Store the thread for later joining
            pthread_join(t3Thread, nullptr);  // Join level 4 thread immediately
        }
    }

        doWork(1000);

    // Join all threads in the loop
    // for (auto& t : threads) {
    //     pthread_join(t, nullptr);
    // }

    std::cout << "Level 3 thread finished after all inner threads. Final value: " << localVar << "\n";
    return nullptr;
}

// Function for level 2 thread
void* taskLevel2(void* arg) {
    std::cout << "Level 2 thread is running.\n";
    pthread_t t3;
    pthread_create(&t3, nullptr, taskLevel3, nullptr);  // Create Level 3 thread
    pthread_join(t3, nullptr);  // Wait for Level 3 to finish
    std::cout << "Level 2 thread finished after level 3.\n";
    return nullptr;
}

// Function for level 1 thread
void* taskLevel1(void* arg) {
    std::cout << "Level 1 thread is running.\n";
    pthread_t t2;
    pthread_create(&t2, nullptr, taskLevel2, nullptr);  // Create Level 2 thread
    pthread_join(t2, nullptr);  // Wait for Level 2 to finish
    std::cout << "Level 1 thread finished.\n";
    return nullptr;
}

// Main function
int main() {
    std::cout << "Main thread is running.\n";

    pthread_t t1;
    pthread_create(&t1, nullptr, taskLevel1, nullptr);  // Create Level 1 thread
    pthread_join(t1, nullptr);  // Wait for Level 1 to finish

    std::cout << "Main thread finished after level 1.\n";
    return 0;
}
