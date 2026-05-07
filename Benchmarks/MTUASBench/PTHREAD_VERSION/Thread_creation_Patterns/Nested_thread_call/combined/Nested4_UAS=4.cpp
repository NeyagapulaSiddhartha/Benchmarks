#include "/home/sidda/FINDUS_Artifact/FINDUS/Benchmarks/MTUASBench/Utilites/doWork.h"

#include <iostream>
#include <pthread.h>
#include <unistd.h>
pthread_t t2C,t3C ,tD1,tD2,t1B,t1C,t1A ,t2A,t2B;
void* taskLevel2A(void* arg) {
    int* ref = static_cast<int*>(arg);
    std::cout << "[A] Level 1 running.\n";
    std::cout << "[A] Level 1 finished. Value: " << *ref << "\n";
    return nullptr;
}
void* taskLevel1A(void* arg) {
    int* ref = static_cast<int*>(arg);
    std::cout << "[A] Level 1 running.\n";
    pthread_create(&t2A, nullptr, taskLevel2A, ref);
    pthread_join(t2A, nullptr);
    std::cout << "[A] Level 1 finished. Value: " << *ref << "\n";
    return nullptr;
}

void* taskLevel0A(void* arg) {
    int localVarA = 111;  // BUG 1: escapes taskLevel0A scope
    std::cout << "[A] Level 0 running. Initial: " << localVarA << "\n";
    pthread_create(&t1A, nullptr, taskLevel1A, &localVarA);
    // pthread_join(t1A, nullptr);  // intentionally omitted
    for (int i = 0; i < 5; i++) {
        for (int j = 0; j < 5; j++) {
            std::cout << "[A] doing work in Level0\n";
            doWork(1000);
        }
        doWork(200);
    }
    std::cout << "[A] Level 0 final: " << localVarA << "\n";
    return nullptr;
}

// ──────────────────────────────────────────────
// BUG 2 — wrong scope join: localVarB owned by taskLevel1B,
//          t2B joined in main instead of taskLevel1B
// ──────────────────────────────────────────────


void* taskLevel2B(void* arg) {
    for (int j = 0; j < 5; j++) {
        for (int jj = 0; jj < 5; jj++) {
            int* y = static_cast<int*>(arg);
            std::cout << "[B-L2] Value: " << *y << "\n";
            std::cout << "[B-L2] doing some work\n";
            doWork(1000);
        }
    }
    return nullptr;
}

void* taskLevel1B(void* arg) {
    int localVarB = 222;  // BUG 2: escapes taskLevel1B scope
    std::cout << "[B] Level 1 running.\n";
    pthread_create(&t2B, nullptr, taskLevel2B, &localVarB);
    // pthread_join(t2B, nullptr);  // intentionally omitted — joined in main
    for (int i = 0; i < 5; i++) {
        for (int j = 0; j < 5; j++) {
            std::cout << "[B] doing work in Level1\n";
            doWork(1000);
        }
        doWork(200);
    }
    std::cout << "[B] Level 1 finished. Final: " << localVarB << "\n";
    return nullptr;
}

// ──────────────────────────────────────────────
// BUG 3 — wrong scope join: localVarC owned by taskLevel2C,
//          t3C joined in taskLevel1C after taskLevel2C returns
// ──────────────────────────────────────────────

void* taskLevel3C(void* arg) {
    for (int j = 0; j < 5; j++) {
        for (int jj = 0; jj < 5; jj++) {
            int* y = static_cast<int*>(arg);
            std::cout << "[C-L3] Value: " << *y << "\n";
            std::cout << "[C-L3] doing some work\n";
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
    std::cout << "[C] Level 2 finished. Final: " << localVarC << "\n";
    return nullptr;
}
void* taskLevel1C(void* arg) {
    std::cout << "[C] Level 1 running.\n";
    pthread_create(&t2C, nullptr, taskLevel2C, nullptr);
    pthread_join(t2C, nullptr);  // t2C done, localVarC already gone
    pthread_join(t3C, nullptr);  // BUG 3: t3C still may access localVarC
    std::cout << "[C] Level 1 finished.\n";
    return nullptr;
}

// ──────────────────────────────────────────────
// BUG 4 — spawn in function, join in main:
//          two independent locals escape spawnD
// ──────────────────────────────────────────────

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

void spawnD() {
    int localVarD = 444;  // BUG 4a: escapes spawnD scope
    int localVarE = 555;  // BUG 4b: escapes spawnD scope
    pthread_create(&tD1, nullptr, UseOfDataD1, &localVarD);
    // pthread_create(&tD2, nullptr, UseOfDataD2, &localVarE);
    for (int i = 0; i < 7; i++) {
        for (int j = 0; j < 5; j++) {
            std::cout << "[D] doing work in spawnD\n";
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

    // BUG 2: taskLevel1B spawns t2B and returns; main joins t2B
    pthread_create(&t1B, nullptr, taskLevel1B, nullptr);

        taskLevel0A(nullptr);

    // BUG 3: localVarC escapes taskLevel2C; t3C joined in taskLevel1C too late
    pthread_create(&t1C, nullptr, taskLevel1C, nullptr);

    for (int i = 0; i < 7; i++) {
        for (int j = 0; j < 5; j++) {
            std::cout << "[D] doing some work in spawnThreadsD\n";
            doWork(1000);
        }
        doWork(200);
    }

    // BUG 4: spawnD returns before tD1/tD2 finish
    spawnD();

    // All remaining joins — after owners have already returned
    pthread_join(t1A,  nullptr);
    pthread_join(t2B,  nullptr);
    pthread_join(tD1,  nullptr);
    pthread_join(tD2,  nullptr);

    std::cout << "Main thread finished.\n";
    return 0;
}