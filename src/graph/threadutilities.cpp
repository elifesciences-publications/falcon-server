#include <unistd.h>

#include "threadutilities.hpp"

bool set_realtime_priority( pthread_t thread, ThreadPriority priority ) {
    
    if (priority<PRIORITY_LOW) {
        return true;
    }
    
    double fraction = static_cast<double>(priority) / 100;
    if (fraction>1) {fraction=1.0;}
    
    auto priority_max = sched_get_priority_max(SCHED_FIFO);
    auto priority_min = sched_get_priority_min(SCHED_FIFO);
    
    // struct sched_param is used to store the scheduling priority
    struct sched_param params;
    // calculate priority value
    params.sched_priority = (int) (fraction * (priority_max-priority_min) + priority_min);
    
    // Attempt to set thread real-time priority to the SCHED_FIFO policy
    if (pthread_setschedparam(thread, SCHED_FIFO, &params)!=0) {
        return false;
    }
    
    // Now verify the change in thread priority
    int policy = 0;
    if (pthread_getschedparam(thread, &policy, &params)!=0) {
        return false;
    }
 
    // Check the correct policy was applied
    if(policy != SCHED_FIFO) {
        return false;
    }
 
    return true;
}

bool set_thread_core( pthread_t thread, ThreadCore core ) {

    int num_cores = sysconf(_SC_NPROCESSORS_ONLN);

    if (core < 0) { return true; }
    
    if (core >= num_cores) { return false; }

    cpu_set_t cpuset;
    
    CPU_ZERO(&cpuset);
    
    CPU_SET(core, &cpuset);
    
    pthread_setaffinity_np(thread, sizeof(cpu_set_t), &cpuset);
    
    return true;
    
}
