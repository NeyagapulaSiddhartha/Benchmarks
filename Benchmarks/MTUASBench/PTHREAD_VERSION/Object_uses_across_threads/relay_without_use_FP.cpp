#include "/home/sidda/FINDUS_Artifact/FINDUS/Benchmarks/MTUASBench/Utilites/doWork.h"
#include <iostream>
#include <pthread.h>
#include <unistd.h> // for 
using namespace std;

pthread_t t1, t2, t3;


void* threadFunc3(void* arg) {

    
    int* x = (int*)arg;
    cout << "[T3] Using value: " << *x << endl;
    return nullptr;
}

// T1: just relays
void* threadFunc2(void* arg) {


    cout << "[T1] Relaying value to T2\n";
    pthread_create(&t2, nullptr, threadFunc3, arg);

        doWork(500000);
    cout << "[T1] T2 gone and it is not owner of data\n";
    return nullptr;
}

// Starts from here
void Func1() {
    int val = 123;  // stack object owned by Func1
    cout << "[Func1] Created int = " << val << endl;

    pthread_create(&t1, nullptr, threadFunc2, &val);
   
   
   
    pthread_join(t1, nullptr);
    pthread_join(t2, nullptr);

    cout << "[Func1] Done\n";
}

int main() {
    cout << "[Main] Calling Func1\n";
    Func1();
   
    cout << "[Main] Exiting\n";
    return 0;
}
