#include "/home/sidda/FINDUS_Artifact/FINDUS/Benchmarks/MTUASBench/Utilites/doWork.h"
/* Description: UAS=4 bugs mixed across all patterns.
   BUG 1 (nested deep pass, 4 levels): localVarA declared in spawnA, passed to
          taskLevel4A. tA joined in main after spawnA returns — UAS.
   BUG 2 (wrong scope join): localVarB owned by taskLevel1B. t2B joined in
          main instead of taskLevel1B — UAS.
   BUG 3 (wrong scope join): localVarC owned by taskLevel2C. t3C joined in
          taskLevel1C instead of taskLevel2C — UAS.
   BUG 4 (spawn + join in main): localVarD and localVarE declared in
          spawnThreadsD; tD1/tD2 joined in main after spawnThreadsD returns — UAS.
*/

#include <iostream>
#include <pthread.h>
#include <unistd.h>

// ──────────────────────────────────────────────
// BUG 1 — 4-level deep pass: localVarA escapes spawnA
// ──────────────────────────────────────────────
pthread_t tA;

void* taskLevel4A(void* arg) {
    for (int j = 0; j < 5; j++) {
        for (int jj = 0; jj < 5; jj++) {
            int* y = static_cast<int*>(arg);
            std::cout << "[A-L4] Value: " << *y << "\n";
            doWork(1000);
        }
    }
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

void spawnA() {
    int localVarA = 111;  // BUG 1: escapes spawnA scope
    std::cout << "[A] spawnA running. Initial value: " << localVarA << "\n";
    pthread_create(&tA, nullptr, taskLevel1A, &localVarA);
    // pthread_join(tA, nullptr);  // intentionally omitted
    for (int i = 0; i < 5; i++) {
        for (int j = 0; j < 5; j++) {
            std::cout << "[A] doing work in spawnA\n";
            doWork(1000);
        }
        doWork(200);
    }
    std::cout << "[A] spawnA done. Final value: " << localVarA << "\n";
}

// ──────────────────────────────────────────────
// BUG 2 — wrong scope join: localVarB owned by taskLevel1B
// ──────────────────────────────────────────────
pthread_t t2B;

void* taskLevel2B(void* arg) {
    for (int j = 0; j < 5; j++) {
        for (int jj = 0; jj < 5; jj++) {
            int* y = static_cast<int*>(arg);
            std::cout << "[B-L2] Value: " << *y << "\n";
            doWork(1000);
        }
    }
    return nullptr;
}

void* taskLevel1B(void* arg) {
    int localVarB = 222;  // BUG 2: escapes taskLevel1B scope
    std::cout << "[B] Level 1 running.\n";
    pthread_create(&t2B, nullptr, taskLevel2B, &localVarB);
    // pthread_join(t2B, nullptr);  // intentionally omitted
    for (int i = 0; i < 5; i++) {
        for (int j = 0; j < 5; j++) {
            std::cout << "[B] doing work in Level1\n";
            doWork(1000);
        }
        doWork(200);
    }
    std::cout << "[B] Level 1 finished. Final value: " << localVarB << "\n";
    return nullptr;
}

// ──────────────────────────────────────────────
// BUG 3 — wrong scope join: localVarC owned by taskLevel2C,
//          t3C joined in taskLevel1C instead of taskLevel2C
// ──────────────────────────────────────────────
pthread_t t3C;

void* taskLevel3C(void* arg) {
    for (int j = 0; j < 5; j++) {
        for (int jj = 0; jj < 5; jj++) {
            int* y = static_cast<int*>(arg);
            std::cout << "[C-L3] Value: " << *y << "\n";
            doWork(1000);
        }
    }
    return nullptr;
}

void* taskLevel2C(void* arg) {
    int localVarC = 333;  // BUG 3: escapes taskLevel2C scope
    std::cout << "[C] Level 2 running.\n";
    pthread_create(&t3C, nullptr, taskLevel3C, &localVarC);
    // pthread_join(t3C, nullptr);  // intentionally omitted — joined in taskLevel1C
    for (int i = 0; i < 5; i++) {
        for (int j = 0; j < 5; j++) {
            std::cout << "[C] doing work in Level2\n";
            doWork(1000);
        }
        doWork(200);
    }
    std::cout << "[C] Level 2 finished. Final value: " << localVarC << "\n";
    return nullptr;
}

void* taskLevel1C(void* arg) {
    std::cout << "[C] Level 1 running.\n";
    pthread_t t2C;
    pthread_create(&t2C, nullptr, taskLevel2C, nullptr);
    pthread_join(t2C, nullptr);  // t2C finishes but localVarC is already gone
    pthread_join(t3C, nullptr);  // BUG 3: t3C joined here, after taskLevel2C returned
    std::cout << "[C] Level 1 finished.\n";
    return nullptr;
}

// ──────────────────────────────────────────────
// BUG 4 — spawn in function, join in main:
//          localVarD escapes spawnThreadsD
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
    int localVarD = 444;  // BUG 4: escapes spawnThreadsD scope
    pthread_create(&tD1, nullptr, UseOfDataD1, &localVarD);
    pthread_create(&tD2, nullptr, UseOfDataD2, &localVarD);
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

    // BUG 1: spawnA returns before tA finishes
    spawnA();

    // BUG 2: taskLevel1B spawns t2B and returns; main joins t2B
    pthread_t t1B;
    pthread_create(&t1B, nullptr, taskLevel1B, nullptr);
    pthread_join(t1B, nullptr);

    // BUG 3: taskLevel2C's localVarC escapes; t3C joined in taskLevel1C
    pthread_t t1C;
    pthread_create(&t1C, nullptr, taskLevel1C, nullptr);
    pthread_join(t1C, nullptr);

    // BUG 4: spawnThreadsD returns before tD1/tD2 finish
    spawnThreadsD();

    // All joins here — after owners have returned
    pthread_join(tA,  nullptr);
    pthread_join(t2B, nullptr);
    pthread_join(tD1, nullptr);
    pthread_join(tD2, nullptr);

    std::cout << "Main thread finished.\n";
    return 0;
}