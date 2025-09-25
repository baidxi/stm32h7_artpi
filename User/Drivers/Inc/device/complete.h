#pragma once

#include <stdbool.h>

struct complete {
    bool complete;
    void (*callback)(void *param);
    void *param;
};