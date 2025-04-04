#pragma once

#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "logger.hh"
#include <string>

namespace task_helpers
{
static void Start_PSRAM_Task(TaskFunction_t function, void* param, const std::string& name, TaskHandle_t& handle, StaticTask_t& buffer, StackType_t** stack, const size_t stack_size, const size_t priority)
{
    *stack = (StackType_t*)heap_caps_malloc(stack_size * sizeof(StackType_t), MALLOC_CAP_SPIRAM);
    if (*stack == NULL)
    {
        NET_LOG_INFO("Failed to allocate stack for %s", name.c_str());
        return;
    }
    handle = xTaskCreateStatic(function, name.c_str(), stack_size, param, priority, *stack, &buffer);

    NET_LOG_WARN("Created moq publish task %s PSRAM left %ld", name.c_str(), heap_caps_get_free_size(MALLOC_CAP_SPIRAM));
}
}