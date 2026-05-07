#include "/home/sidda/FINDUS_Artifact/FINDUS/Benchmarks/MTUASBench/Utilites/doWork.h"
/* Description: In this program a function call chain is created like main->a()->b()->c()->runThread.
   A data object x is passed as reference from one function to another starting from function c(). 
   In function c local variable is passed to final function in the chain runThread where it is Used. 
   Thread t1 is joined in c() function preventing Use After Scope */

#include <iostream>
#include <pthread.h>
using namespace std;

pthread_t t1;

void* runThread(void *arg) {
    int *t = static_cast<int*>(arg);
    cout << "Value passed from func b:" << *t << endl;
    return nullptr;
}

void c() {
    int r = 1;
    if (r == 10) {
        // Do nothing
    } else {
        pthread_create(&t1, nullptr, runThread, static_cast<void*>(&r));
        pthread_join(t1, nullptr); // Join the thread here to prevent Use After Scope
    }
}

void b() {
    int y = 10;
    c();
}

void a() {
    b();
}

int main() {
    a();
}
