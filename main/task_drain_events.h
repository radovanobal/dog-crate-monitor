#ifndef DOG_CRATE_MONITOR_TASK_DRAIN_EVENTS_H
#define DOG_CRATE_MONITOR_TASK_DRAIN_EVENTS_H

typedef enum {
    TASK_DRAIN_STATE_IDLE,
    TASK_DRAIN_STATE_DRAINING,
    TASK_DRAIN_STATE_COMPLETE
} TaskDrainState;

typedef enum {
    TASK_DRAIN_TYPE_RENDER,
    TASK_DRAIN_TYPE_UI,
    TASK_DRAIN_TYPE_SERVICE
} TaskDrainType;

typedef void (*TaskDrainStateChangedCallback)(TaskDrainState state);

void taskDrainMonitor_taskStatusChanged(TaskDrainType drainType, TaskDrainState state);
void taskDrainMonitor_registerDrainCompleteCallback(TaskDrainStateChangedCallback callback);

#endif // DOG_CRATE_MONITOR_TASK_DRAIN_EVENTS_H