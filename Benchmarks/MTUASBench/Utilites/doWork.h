#ifndef DOWORK_H
#define DOWORK_H

#ifdef __cplusplus
extern "C" {
#endif

int  getWorkDuration();
int  getStepDelay(int base_time_us);
void doWork(int base_time_us);

#ifdef __cplusplus
}
#endif

#endif // DOWORK_H