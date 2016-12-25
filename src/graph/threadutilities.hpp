#ifndef THREADUTILITIES_H
#define THREADUTILITIES_H

#include <thread>

typedef int16_t ThreadPriority;

const ThreadPriority PRIORITY_NONE = -1;
const ThreadPriority PRIORITY_LOW = 0;
const ThreadPriority PRIORITY_MEDIUM = 50;
const ThreadPriority PRIORITY_HIGH = 100;

typedef int16_t ThreadCore;

const ThreadCore CORE_NOT_PINNED = -1;

bool set_realtime_priority( pthread_t thread, ThreadPriority priority = PRIORITY_NONE );

bool set_thread_core( pthread_t thread, ThreadCore core );

#endif
