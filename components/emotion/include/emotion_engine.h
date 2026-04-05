#pragma once

typedef enum {
    ROBOT_STATE_BOOTING = 0,
    ROBOT_STATE_IDLE,
    ROBOT_STATE_LISTENING,
    ROBOT_STATE_PROCESSING,
    ROBOT_STATE_SPEAKING,
    ROBOT_STATE_HAPPY,
    ROBOT_STATE_ERROR,
    ROBOT_STATE_SLEEP,
} robot_state_t;

void emotion_engine_init(void);
void emotion_engine_request_state(robot_state_t requested_state);
void emotion_engine_update(void);
robot_state_t emotion_engine_get_state(void);
const char *emotion_engine_state_name(robot_state_t state);
