#pragma once

#include "emotion_engine.h"

#ifdef __cplusplus
extern "C" {
#endif

void display_service_init(void);
void display_service_render(robot_state_t state);

#ifdef __cplusplus
}
#endif
