#pragma once
#include <stdbool.h>
#include <stdint.h>

typedef void (*event_func_t)(uint32_t type, void *arg);

typedef struct event {
    uint32_t type;
    event_func_t fn;
    void *arg;
    bool _posted;
    struct event *_next;
} event_t;

void event_init(event_func_t generic_event_handler);
bool event_execute_one(void);
void event_add(event_t *e, uint32_t type, void *arg);
