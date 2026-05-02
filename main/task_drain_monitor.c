#include <stddef.h>

#include "esp_log.h"

#include "task_drain_monitor.h"
#include "task_drain_events.h"


#define MAX_MONITORED_QUEUES 2
#define MAX_MONITORED_RUNNING_TASKERS 3

typedef struct {
    QueueHandle_t monitoredQueues[MAX_MONITORED_QUEUES];
    size_t queueCount;
} TaskDrainQueueMonitorState;

typedef struct {
    TaskDrainType drainType;
    TaskDrainState state;
} TaskDrainStatus;

typedef struct {
    TaskDrainStatus monitoredTaskers[MAX_MONITORED_RUNNING_TASKERS];
    size_t runningTaskCount;
} TaskDrainTaskList;

static const char *TAG = "task_drain_monitor";

static size_t findTaskerIndex(TaskDrainType drainType);

static TaskDrainTaskList taskList = { 
    . monitoredTaskers = {
        { .drainType = TASK_DRAIN_TYPE_RENDER, .state = TASK_DRAIN_STATE_IDLE },
        { .drainType = TASK_DRAIN_TYPE_UI, .state = TASK_DRAIN_STATE_IDLE },
        { .drainType = TASK_DRAIN_TYPE_SERVICE, .state = TASK_DRAIN_STATE_IDLE }
    },
    .runningTaskCount = 0 
};

static TaskDrainQueueMonitorState monitorState = { .queueCount = 0 };
static TaskDrainStateChangedCallback drainCompleteCallback = NULL;

void taskDrainMonitor_registerDrainCompleteCallback(TaskDrainStateChangedCallback callback) {
    drainCompleteCallback = callback;
}

void taskDrainMonitor_setTaskQueueToMonitor(QueueHandle_t taskQueue) {
    if (monitorState.queueCount < MAX_MONITORED_QUEUES) {
        monitorState.monitoredQueues[monitorState.queueCount++] = taskQueue;
    }
}

void taskDrainMonitor_taskStatusChanged(TaskDrainType drainType, TaskDrainState state) {
    const size_t taskerIndex = findTaskerIndex(drainType);

    if (taskerIndex == -1) {
        ESP_LOGW(TAG, "Received drain status update for unrecognized task type: %d", drainType);

        return;
    }

    taskList.monitoredTaskers[taskerIndex].state = state;

    if (state != TASK_DRAIN_STATE_IDLE)  {
        return; // only care about transitions to idle for now
    }

    // Check if all monitored taskers are idle
    bool allIdle = true;
    for (size_t i = 0; i < MAX_MONITORED_RUNNING_TASKERS; i++) {
        if (taskList.monitoredTaskers[i].state != TASK_DRAIN_STATE_IDLE) {
            allIdle = false;

            break;
        }
    }

    bool queuesCleared = false;
    for (size_t i = 0; i < monitorState.queueCount; i++) {
        // Clear any pending work in the monitored queues to prevent stale work from running after drain complete
        uxQueueMessagesWaiting(monitorState.monitoredQueues[i]);

        if (uxQueueMessagesWaiting(monitorState.monitoredQueues[i]) > 0) {
            queuesCleared = true;
        }
    }

    if (drainCompleteCallback == NULL) {
        return;
    }

    if (allIdle && !queuesCleared) {
        drainCompleteCallback(TASK_DRAIN_STATE_COMPLETE);
    }
}

static size_t findTaskerIndex(TaskDrainType drainType) {
    for (size_t i = 0; i < MAX_MONITORED_RUNNING_TASKERS; i++) {
        if (taskList.monitoredTaskers[i].drainType == drainType) {
            return i;
        }
    }

    return -1;
}