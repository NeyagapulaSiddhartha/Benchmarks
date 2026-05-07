#include "/home/sidda/FINDUS_Artifact/FINDUS/Benchmarks/MTUASBench/Utilites/doWork.h"
/* Description: UAS=4 bugs mixed across all patterns.
   BUG 1 (nested deep pass, 5 levels): localVarA declared in taskLevel0A, passed
          all the way to taskLevel5A. t1A joined in main instead of taskLevel0A — UAS.
   BUG 2 (nested deep pass, 3 levels): localVarB declared in taskLevel0B, passed
          to taskLevel3B. t1B joined in main instead of taskLevel0B — UAS.
   BUG 3 (wrong scope join): localVarC owned by taskLevel1C. t2C joined in
          main instead of taskLevel1C — UAS.
   BUG 4 (spawn + join in main): two locals declared in spawnThreadsD;
          tD1 and tD2 use them; joined in main after spawnThreadsD returns — UAS.
*/

#include <iostream>
#include <pthread.h>
#include <unistd.h>

// ──────────────────────────────────────────────
// BUG 1 — 5-level deep pass: localVarA escapes taskLevel0A
// ──────────────────────────────────────────────
pthread_t t1A;

void* taskLevel5A(void* arg) {
    for (int j = 0; j < 5; j++) {
        for (int jj = 0; jj < 5; jj++) {
            int* y = static_cast<int*>(arg);
            std::cout << "[A-L5] Value: " << *y << "\n";
            std::cout << "[A-L5] doing some work\n";
            doWork(1000);
        }
    }
    return nullptr;
}

void* taskLevel4A(void* arg) {
    int* ref = static_cast<int*>(arg);
    std::cout << "[A] Level 4 running.\n";
    pthread_t t5A;
    pthread_create(&t5A, nullptr, taskLevel5A, ref);
    pthread_join(t5A, nullptr);
    std::cout << "[A] Level 4 finished. Value: " << *ref << "\n";
    return nullptr;
}

void* taskLevel3A(void* arg) {
    int* ref = static_cast<int*>(arg);
    std::cout << "[A] Level 3 running.\n";
    pthread_t t4A;
    pthread_create(&t4A, nullptr, taskLevel4A, ref);
    pthread_join(t4A, nullptr);
    std::cout << "[A] Level 3 finished. Value: " << *ref << "\n";
    return nullptr;
}

void* taskLevel2A(void* arg) {
    int* ref = static_cast<int*>(arg);
    std::cout << "[A] Level 2 running.\n";
    pthread_t t3A;
    pthread_create(&t3A, nullptr, taskLevel3A, ref);
    pthread_join(t3A, nullptr);
    std::cout << "[A] Level 2 finished. Value: " << *ref << "\n";
    return nullptr;
}

void* taskLevel1A(void* arg) {
    int* ref = static_cast<int*>(arg);
    std::cout << "[A] Level 1 running.\n";
    pthread_t t2A;
    pthread_create(&t2A, nullptr, taskLevel2A, ref);
    pthread_join(t2A, nullptr);
    std::cout << "[A] Level 1 finished. Value: " << *ref << "\n";
    return nullptr;
}

void* taskLevel0A(void* arg) {
    int localVarA = 111;  // BUG 1: escapes taskLevel0A scope
    std::cout << "[A] Level 0 running. Initial value: " << localVarA << "\n";
    pthread_create(&t1A, nullptr, taskLevel1A, &localVarA);
    // pthread_join(t1A, nullptr);  // intentionally omitted
    for (int i = 0; i < 5; i++) {
        for (int j = 0; j < 5; j++) {
            std::cout << "[A] doing work in Level0\n";
            doWork(1000);
        }
        doWork(200);
    }
    std::cout << "[A] Level 0 final value: " << localVarA << "\n";
    return nullptr;
}

// ──────────────────────────────────────────────
// BUG 2 — 3-level deep pass: localVarB escapes taskLevel0B
// ──────────────────────────────────────────────
pthread_t t1B;

void* taskLevel3B(void* arg) {
    for (int j = 0; j < 5; j++) {
        for (int jj = 0; jj < 5; jj++) {
            int* y = static_cast<int*>(arg);
            std::cout << "[B-L3] Value: " << *y << "\n";
            std::cout << "[B-L3] doing some work\n";
            doWork(1000);
        }
    }
    return nullptr;
}

void* taskLevel2B(void* arg) {
    int* ref = static_cast<int*>(arg);
    std::cout << "[B] Level 2 running.\n";
    pthread_t t3B;
    pthread_create(&t3B, nullptr, taskLevel3B, ref);
    pthread_join(t3B, nullptr);
    std::cout << "[B] Level 2 finished. Value: " << *ref << "\n";
    return nullptr;
}

void* taskLevel1B(void* arg) {
    int* ref = static_cast<int*>(arg);
    std::cout << "[B] Level 1 running.\n";
    pthread_t t2B;
    pthread_create(&t2B, nullptr, taskLevel2B, ref);
    pthread_join(t2B, nullptr);
    std::cout << "[B] Level 1 finished. Value: " << *ref << "\n";
    return nullptr;
}

void* taskLevel0B(void* arg) {
    int localVarB = 222;  // BUG 2: escapes taskLevel0B scope
    std::cout << "[B] Level 0 running. Initial value: " << localVarB << "\n";
    pthread_create(&t1B, nullptr, taskLevel1B, &localVarB);
    // pthread_join(t1B, nullptr);  // intentionally omitted
    for (int i = 0; i < 5; i++) {
        for (int j = 0; j < 5; j++) {
            std::cout << "[B] doing work in Level0\n";
            doWork(1000);
        }
        doWork(200);
    }
    std::cout << "[B] Level 0 final value: " << localVarB << "\n";
    return nullptr;
}

// ──────────────────────────────────────────────
// BUG 3 — wrong scope join: localVarC owned by taskLevel1C,
//          t2C joined in main instead of taskLevel1C
// ──────────────────────────────────────────────
pthread_t t2C;

void* taskLevel2C(void* arg) {
    for (int j = 0; j < 5; j++) {
        for (int jj = 0; jj < 5; jj++) {
            int* y = static_cast<int*>(arg);
            std::cout << "[C-L2] Value: " << *y << "\n";
            std::cout << "[C-L2] doing some work\n";
            doWork(1000);
        }
    }
    return nullptr;
}

void* taskLevel1C(void* arg) {
    int localVarC = 333;  // BUG 3: escapes taskLevel1C scope
    std::cout << "[C] Level 1 running.\n";
    pthread_create(&t2C, nullptr, taskLevel2C, &localVarC);
    // pthread_join(t2C, nullptr);  // intentionally omitted — joined in main
    for (int i = 0; i < 5; i++) {
        for (int j = 0; j < 5; j++) {
            std::cout << "[C] doing work in Level1\n";
            doWork(1000);
        }
        doWork(200);
    }
    std::cout << "[C] Level 1 finished. Final value: " << localVarC << "\n";
    return nullptr;
}

// ──────────────────────────────────────────────
// BUG 4 — spawn in function, join in main:
//          two separate locals x and y escape spawnThreadsD
// ──────────────────────────────────────────────
pthread_t tD1, tD2;

void* UseOfDataD1(void* arg) {
    for (int j = 0; j < 5; j++) {
        for (int jj = 0; jj < 5; jj++) {
            int* y = static_cast<int*>(arg);
            std::cout << "[D1] Value: " << *y << "\n";
            doWork(1000);
        }
    }
    return nullptr;
}

void* UseOfDataD2(void* arg) {
    for (int j = 0; j < 5; j++) {
        for (int jj = 0; jj < 5; jj++) {
            int* y = static_cast<int*>(arg);
            std::cout << "[D2] Value: " << *y << "\n";
            doWork(1000);
        }
    }
    return nullptr;
}

void spawnThreadsD() {
    int x = 444;  // BUG 4a: escapes spawnThreadsD
    int y = 555;  // BUG 4b: escapes spawnThreadsD
    pthread_create(&tD1, nullptr, UseOfDataD1, &x);
    pthread_create(&tD2, nullptr, UseOfDataD2, &y);
    for (int i = 0; i < 7; i++) {
        for (int j = 0; j < 5; j++) {
            std::cout << "[D] doing work in spawnThreadsD\n";
            doWork(1000);
        }
        doWork(200);
    }
    // pthread_join(tD1, nullptr);  // intentionally omitted
    // pthread_join(tD2, nullptr);  // intentionally omitted
}

// ──────────────────────────────────────────────
// Main
// ──────────────────────────────────────────────
int main() {
    std::cout << "Main thread starting.\n";

    // BUG 1: taskLevel0A returns before t1A finishes
    taskLevel0A(nullptr);

    // BUG 2: taskLevel0B returns before t1B finishes
    taskLevel0B(nullptr);

    // BUG 3: taskLevel1C returns before t2C finishes; main joins t2C
    pthread_t t1C;
    pthread_create(&t1C, nullptr, taskLevel1C, nullptr);
    pthread_join(t1C, nullptr);

    // BUG 4: spawnThreadsD returns before tD1/tD2 finish
    spawnThreadsD();

    // All joins here — after owners have returned
    pthread_join(t1A, nullptr);
    pthread_join(t1B, nullptr);
    pthread_join(t2C, nullptr);
    pthread_join(tD1, nullptr);
    pthread_join(tD2, nullptr);

    std::cout << "Main thread finished.\n";
    return 0;
}