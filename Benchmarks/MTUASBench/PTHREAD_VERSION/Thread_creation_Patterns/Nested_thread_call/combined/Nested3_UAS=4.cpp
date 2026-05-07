#include "/home/sidda/FINDUS_Artifact/FINDUS/Benchmarks/MTUASBench/Utilites/doWork.h"


#include <iostream>
#include <pthread.h>
#include <unistd.h>

// ──────────────────────────────────────────────
// BUG 1 — deep nested pass: localVarA escapes taskLevel0A
// ──────────────────────────────────────────────
pthread_t t1A;

void* taskLevel3A(void* arg) {
    for (int j = 0; j < 5; j++) {
        for (int jj = 0; jj < 5; jj++) {
            int* y = static_cast<int*>(arg);
            std::cout << "[A-L3] Value of y is: " << *y << "\n";
            std::cout << "[A-L3] doing some work\n";
            doWork(1000);
        }
    }
    return nullptr;
}

void* taskLevel2A(void* arg) {
    int* ref = static_cast<int*>(arg);
    std::cout << "[A] Level 2 thread is running.\n";
    pthread_t t3A;
    pthread_create(&t3A, nullptr, taskLevel3A, ref);
    pthread_join(t3A, nullptr);
    std::cout << "[A] Level 2 finished. Current value: " << *ref << "\n";
    return nullptr;
}

void* taskLevel1A(void* arg) {
    int* ref = static_cast<int*>(arg);
    std::cout << "[A] Level 1 thread is running.\n";
    pthread_t t2A;
    pthread_create(&t2A, nullptr, taskLevel2A, ref);
    std::cout << "[A] Level 1 finished. Current value: " << *ref << "\n";
    return nullptr;
}

void* taskLevel0A(void* arg) {
    int localVarA = 111;  // BUG 1: this escapes scope
    std::cout << "[A] Level 0 running. Initial value: " << localVarA << "\n";
    pthread_create(&t1A, nullptr, taskLevel1A, &localVarA);
        pthread_join(t1A, nullptr);

    // pthread_join(t1A, nullptr);  // intentionally omitted — join happens in main
    for (int i = 0; i < 5; i++) {
        for (int j = 0; j < 5; j++) {
            std::cout << "[A] doing some work in Level0\n";
            doWork(1000);
        }
        doWork(200);
    }
    std::cout << "[A] Level 0 final value: " << localVarA << "\n";
    return nullptr;
}

// ──────────────────────────────────────────────
// BUG 2 — deep nested pass: localVarB escapes taskLevel0B
// ──────────────────────────────────────────────
pthread_t t1B;

void* taskLevel3B(void* arg) {
    for (int j = 0; j < 5; j++) {
        for (int jj = 0; jj < 5; jj++) {
            int* y = static_cast<int*>(arg);
            std::cout << "[B-L3] Value of y is: " << *y << "\n";
            std::cout << "[B-L3] doing some work\n";
            doWork(1000);
        }
    }
    return nullptr;
}

void* taskLevel2B(void* arg) {
    int* ref = static_cast<int*>(arg);
    std::cout << "[B] Level 2 thread is running.\n";
    pthread_t t3B;
    pthread_create(&t3B, nullptr, taskLevel3B, ref);
        pthread_join(t3B, nullptr);

    std::cout << "[B] Level 2 finished. Current value: " << *ref << "\n";
    return nullptr;
}

void* taskLevel1B(void* arg) {
    int* ref = static_cast<int*>(arg);
    std::cout << "[B] Level 1 thread is running.\n";
    pthread_t t2B;
    pthread_create(&t2B, nullptr, taskLevel2B, ref);
    pthread_join(t2B, nullptr);

    
    std::cout << "[B] Level 1 finished. Current value: " << *ref << "\n";
    return nullptr;
}

void* taskLevel0B(void* arg) {
    int localVarB = 222;  // BUG 2: this escapes scope
    std::cout << "[B] Level 0 running. Initial value: " << localVarB << "\n";
    pthread_create(&t1B, nullptr, taskLevel1B, &localVarB);
    pthread_join(t1B, nullptr);
    // pthread_join(t1B, nullptr);  // intentionally omitted — join happens in main
    for (int i = 0; i < 5; i++) {
        for (int j = 0; j < 5; j++) {
            std::cout << "[B] doing some work in Level0\n";
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
            std::cout << "[C-L2] Value of y is: " << *y << "\n";
            std::cout << "[C-L2] doing some work\n";
            doWork(1000);
        }
    }
    return nullptr;
}

void* taskLevel1C(void* arg) {
    int localVarC = 333;  // BUG 3: this escapes scope
    std::cout << "[C] Level 1 running.\n";
    pthread_create(&t2C, nullptr, taskLevel2C, &localVarC);
    pthread_join(t2C, nullptr);  // intentionally omitted — join happens in main
    for (int i = 0; i < 5; i++) {
        for (int j = 0; j < 5; j++) {
            std::cout << "[C] doing some work in Level1\n";
            doWork(1000);
        }
        doWork(200);
    }
    std::cout << "[C] Level 1 finished. Final value: " << localVarC << "\n";
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
            std::cout << "[D1] Value of y is: " << *y << "\n";
            std::cout << "[D1] doing some work\n";
            doWork(1000);
        }
    }
    return nullptr;
}

void* UseOfDataD2(void* arg) {
    for (int j = 0; j < 5; j++) {
        for (int jj = 0; jj < 5; jj++) {
            int* y = static_cast<int*>(arg);
            std::cout << "[D2] Value of y is: " << *y << "\n";
            std::cout << "[D2] doing some work\n";
            doWork(1000);
        }
    }
    return nullptr;
}

void spawnThreadsD() {
    int localVarD = 444;  // BUG 4: this escapes scope
    pthread_create(&tD1, nullptr, UseOfDataD1, &localVarD);
    pthread_create(&tD2, nullptr, UseOfDataD2, &localVarD);
    for (int i = 0; i < 7; i++) {
        for (int j = 0; j < 5; j++) {
            std::cout << "[D] doing some work in spawnThreadsD\n";
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

    // BUG 1 & 2: taskLevel0A/B return before t1A/t1B finish


    // BUG 3: taskLevel1C spawns t2C; t1C finishes before t2C, then main joins t2C
    pthread_t t1C;
    pthread_create(&t1C, nullptr, taskLevel1C, nullptr);

        taskLevel0A(nullptr);
    taskLevel0B(nullptr);
    
    pthread_join(t1C, nullptr);

    // BUG 4: spawnThreadsD returns before tD1/tD2 finish
    spawnThreadsD();
        for (int i = 0; i < 7; i++) {
        for (int j = 0; j < 5; j++) {
            std::cout << "[D] doing some work in spawnThreadsD\n";
            doWork(1000);
        }
        doWork(200);
    }

    // All joins happen in main — after owners have returned
    pthread_join(t1A, nullptr);
    pthread_join(t1B, nullptr);
    pthread_join(t2C, nullptr);
    pthread_join(tD1, nullptr);
    pthread_join(tD2, nullptr);

    std::cout << "Main thread finished.\n";
    return 0;
}