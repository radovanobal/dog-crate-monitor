#ifndef DOG_CRATE_MONITOR_DRAIN_MONITOR_H
#define DOG_CRATE_MONITOR_DRAIN_MONITOR_H

#include "freertos/idf_additions.h"

void taskDrainMonitor_setTaskQueueToMonitor(QueueHandle_t taskQueue);

#endif // DOG_CRATE_MONITOR_DRAIN_MONITOR_H
