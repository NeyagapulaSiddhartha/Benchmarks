#include "/home/sidda/FINDUS_Artifact/FINDUS/Benchmarks/MTUASBench/Utilites/doWork.h"




#include <iostream>
#include <pthread.h>
#include <unistd.h> // for 
using namespace std;
int *gPtr;
pthread_t t1,t2;

void* t2_func(void* arg) {
    printf("T2 read: %d\n", *gPtr);
    return NULL;
}

void* t1_func(void* arg) {
    int local = 30;
    gPtr = &local;
    pthread_create(&t2, NULL, t2_func, NULL);
    pthread_join(t2, NULL); // T2 => T1
    return NULL;
}

int main() {
    pthread_create(&t1, NULL, t1_func, NULL);
    pthread_join(t1, NULL);
    return 0;
}
