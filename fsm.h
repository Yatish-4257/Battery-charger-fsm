/**
 * @file fsm.h
 * @brief Battery charging controller finite state machine interface.
 *
 * Table-driven FSM mapping (state, event) to (next state, transition action).
 * Per-state entry and exit hooks are invoked by the dispatch engine in fsm.c.
 */

#ifndef BATTERY_CHARGER_FSM_H
#define BATTERY_CHARGER_FSM_H

#include <stdbool.h>

/**
 * @brief Operational states of the charging controller.
 */
typedef enum {
    FSM_STATE_IDLE = 0,
    FSM_STATE_TRICKLE_CHARGE,
    FSM_STATE_CONSTANT_CURRENT,
    FSM_STATE_CONSTANT_VOLTAGE,
    FSM_STATE_FULL,
    FSM_STATE_FAULT,
    FSM_STATE_COUNT
} FsmState_t;

/**
 * @brief Events that drive state transitions.
 */
typedef enum {
    FSM_EVENT_CHARGER_CONNECTED = 0,
    FSM_EVENT_VOLTAGE_THRESHOLD_REACHED,
    FSM_EVENT_CURRENT_THRESHOLD_REACHED,
    FSM_EVENT_BATTERY_FULL,
    FSM_EVENT_OVERTEMPERATURE_DETECTED,
    FSM_EVENT_CHARGER_DISCONNECTED,
    FSM_EVENT_COUNT
} FsmEvent_t;

/**
 * @brief Runtime FSM context.
 */
typedef struct {
    FsmState_t currentState;
} FsmContext_t;

/**
 * @brief Initialize context and enter the Idle state.
 * @param ctx  Non-null FSM context pointer.
 */
void Fsm_Init(FsmContext_t *ctx);

/**
 * @brief Dispatch an event against the current state.
 * @param ctx    Non-null FSM context pointer.
 * @param event  Incoming event.
 * @return true if a valid transition was executed, false if ignored.
 */
bool Fsm_HandleEvent(FsmContext_t *ctx, FsmEvent_t event);

/**
 * @brief Return the current operational state.
 */
FsmState_t Fsm_GetState(const FsmContext_t *ctx);

/**
 * @brief Human-readable state name for logging and test output.
 */
const char *Fsm_StateName(FsmState_t state);

/**
 * @brief Human-readable event name for logging and test output.
 */
const char *Fsm_EventName(FsmEvent_t event);

#endif /* BATTERY_CHARGER_FSM_H */
