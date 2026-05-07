#include "/home/sidda/FINDUS_Artifact/FINDUS/Benchmarks/MTUASBench/Utilites/doWork.h"
#include <iostream>
#include <pthread.h>
#include <unistd.h>
using namespace std;

int *p = nullptr, *q = nullptr, *r = nullptr;

pthread_t threadA, threadB, threadC, threadD;
bool createdA = false;

// Protect join-once behavior
pthread_mutex_t join_mutex = PTHREAD_MUTEX_INITIALIZER;
bool isAJoined = false;

void joinAOnce(const char* who) {
    pthread_mutex_lock(&join_mutex);
    if (!isAJoined && createdA) {
        pthread_join(threadA, nullptr);
        isAJoined = true;
        cout << "[" << who << "] Joined A\n";
    }
    pthread_mutex_unlock(&join_mutex);
}

// D assigns r and joins A
void* funcD(void* arg) {
    int c = 400;
    r = &c;
    cout << "[D] Assigned r = &c (" << c << ")\n";
    joinAOnce("D");
    return nullptr;
}

// C assigns q and joins A
void* funcC(void* arg) {
    int qc = 300;
    q = &qc;
    cout << "[C] Assigned q = &qc (" << qc << ")\n";
    joinAOnce("C");
    return nullptr;
}

// B assigns p and joins A
void* funcB(void* arg) {
    int pb = 200;
    p = &pb;
    cout << "[B] Assigned p = &pb (" << pb << ")\n";
    joinAOnce("B");
    return nullptr;
}

// A uses p, q, r
void* funcA(void* arg) {
    if (p) cout << "[A] Using *p = " << *p << " (may be UAS!)\n";
    if (q) cout << "[A] Using *q = " << *q << " (may be UAS!)\n";
    if (r) cout << "[A] Using *r = " << *r << " (may be UAS!)\n";
    return nullptr;
}

int main() {
    pthread_create(&threadB, nullptr, funcB, nullptr);
    pthread_create(&threadC, nullptr, funcC, nullptr);
    pthread_create(&threadD, nullptr, funcD, nullptr);

    if (pthread_create(&threadA, nullptr, funcA, nullptr) == 0) {
        createdA = true;
        cout << "[Main] Started Thread A\n";
    }

    // Join only B, C, D — A will be joined once by one of them
    pthread_join(threadB, nullptr);
    pthread_join(threadC, nullptr);
    pthread_join(threadD, nullptr);

    cout << "[Main] B, C, D done — one of them joined A safely\n";
    return 0;
}
