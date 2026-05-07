#include "doWork.h"
#include <cstdlib>
#include <ctime>
#include <unistd.h>

    int getWorkDuration() {
    return 10 + (rand() % 8);   // 3–10 iterations
}

 int getStepDelay(int base_time_us) {
    if (base_time_us <= 0) return 0;
    // (-1)^rand() — randomly increase or decrease around base
    int sign = (rand() % 10 < 5) ? -1 : 1;  // 30% faster, 70% slower
    int variation = rand() % base_time_us;
    int result = base_time_us + sign * variation;
    return (result > 0) ? result : 1;
}

void doWork(int base_time_us) {
    static bool _dowork_seeded = false;
    if (!_dowork_seeded) {
        srand(static_cast<unsigned>(time(nullptr) ^ static_cast<unsigned>(getpid())));
        _dowork_seeded = true;
    }
    int _work = getWorkDuration();
    int sign = (rand() % 10 < 5) ? -1 : 1;  // 30% faster, 70% slower

    for (int _j = 0; _j < _work + sign*10; _j++) {
    int _work = getWorkDuration();
    int sign = (rand() % 10 < 5) ? -1 : 1;  // 30% faster, 70% slower
        for (int _j = 0; _j < _work + sign*10; _j++) {
             usleep(static_cast<useconds_t>(getStepDelay(base_time_us)));
        }
    }
}