/**
 * @file fsm.c
 * @brief Transition table and FSM dispatch engine.
 */

#include "fsm.h"

#include <stddef.h>

/* Transition and state hook function pointer types. */
typedef void (*FsmActionFn_t)(void);

typedef struct {
    FsmState_t    nextState;
    FsmActionFn_t action;
} FsmTransition_t;

typedef struct {
    FsmActionFn_t onEntry;
    FsmActionFn_t onExit;
} FsmStateDef_t;

/* Forward declarations — implemented in actions.c */
void Fsm_OnEnter_Idle(void);
void Fsm_OnEnter_TrickleCharge(void);
void Fsm_OnEnter_ConstantCurrent(void);
void Fsm_OnEnter_ConstantVoltage(void);
void Fsm_OnEnter_Full(void);
void Fsm_OnEnter_Fault(void);

void Fsm_OnExit_Idle(void);
void Fsm_OnExit_TrickleCharge(void);
void Fsm_OnExit_ConstantCurrent(void);
void Fsm_OnExit_ConstantVoltage(void);
void Fsm_OnExit_Full(void);
void Fsm_OnExit_Fault(void);

void Fsm_Action_StartTrickleCharge(void);
void Fsm_Action_BeginConstantCurrent(void);
void Fsm_Action_BeginConstantVoltage(void);
void Fsm_Action_TerminateCharge(void);
void Fsm_Action_StopCharging(void);
void Fsm_Action_ReportOvertemperature(void);
void Fsm_Action_ClearFault(void);

/** Sentinel: nextState == FSM_STATE_COUNT means no valid transition. */
#define FSM_NO_TRANSITION { FSM_STATE_COUNT, NULL }

static const FsmStateDef_t stateTable[FSM_STATE_COUNT] = {
    [FSM_STATE_IDLE]              = { Fsm_OnEnter_Idle,              Fsm_OnExit_Idle },
    [FSM_STATE_TRICKLE_CHARGE]      = { Fsm_OnEnter_TrickleCharge,     Fsm_OnExit_TrickleCharge },
    [FSM_STATE_CONSTANT_CURRENT]  = { Fsm_OnEnter_ConstantCurrent,   Fsm_OnExit_ConstantCurrent },
    [FSM_STATE_CONSTANT_VOLTAGE]  = { Fsm_OnEnter_ConstantVoltage,   Fsm_OnExit_ConstantVoltage },
    [FSM_STATE_FULL]              = { Fsm_OnEnter_Full,              Fsm_OnExit_Full },
    [FSM_STATE_FAULT]             = { Fsm_OnEnter_Fault,             Fsm_OnExit_Fault },
};

/**
 * Transition table indexed by [currentState][event].
 * Overtemperature is handled from every active charging state.
 */
static const FsmTransition_t transitionTable[FSM_STATE_COUNT][FSM_EVENT_COUNT] = {
    [FSM_STATE_IDLE] = {
        [FSM_EVENT_CHARGER_CONNECTED]           = { FSM_STATE_TRICKLE_CHARGE,     Fsm_Action_StartTrickleCharge },
        [FSM_EVENT_VOLTAGE_THRESHOLD_REACHED]   = FSM_NO_TRANSITION,
        [FSM_EVENT_CURRENT_THRESHOLD_REACHED]   = FSM_NO_TRANSITION,
        [FSM_EVENT_BATTERY_FULL]                = FSM_NO_TRANSITION,
        [FSM_EVENT_OVERTEMPERATURE_DETECTED]    = FSM_NO_TRANSITION,
        [FSM_EVENT_CHARGER_DISCONNECTED]        = FSM_NO_TRANSITION,
    },
    [FSM_STATE_TRICKLE_CHARGE] = {
        [FSM_EVENT_CHARGER_CONNECTED]           = FSM_NO_TRANSITION,
        [FSM_EVENT_VOLTAGE_THRESHOLD_REACHED]   = { FSM_STATE_CONSTANT_CURRENT,   Fsm_Action_BeginConstantCurrent },
        [FSM_EVENT_CURRENT_THRESHOLD_REACHED]   = FSM_NO_TRANSITION,
        [FSM_EVENT_BATTERY_FULL]                = FSM_NO_TRANSITION,
        [FSM_EVENT_OVERTEMPERATURE_DETECTED]    = { FSM_STATE_FAULT,              Fsm_Action_ReportOvertemperature },
        [FSM_EVENT_CHARGER_DISCONNECTED]        = { FSM_STATE_IDLE,               Fsm_Action_StopCharging },
    },
    [FSM_STATE_CONSTANT_CURRENT] = {
        [FSM_EVENT_CHARGER_CONNECTED]           = FSM_NO_TRANSITION,
        [FSM_EVENT_VOLTAGE_THRESHOLD_REACHED]   = { FSM_STATE_CONSTANT_VOLTAGE,   Fsm_Action_BeginConstantVoltage },
        [FSM_EVENT_CURRENT_THRESHOLD_REACHED]   = FSM_NO_TRANSITION,
        [FSM_EVENT_BATTERY_FULL]                = FSM_NO_TRANSITION,
        [FSM_EVENT_OVERTEMPERATURE_DETECTED]    = { FSM_STATE_FAULT,              Fsm_Action_ReportOvertemperature },
        [FSM_EVENT_CHARGER_DISCONNECTED]        = { FSM_STATE_IDLE,               Fsm_Action_StopCharging },
    },
    [FSM_STATE_CONSTANT_VOLTAGE] = {
        [FSM_EVENT_CHARGER_CONNECTED]           = FSM_NO_TRANSITION,
        [FSM_EVENT_VOLTAGE_THRESHOLD_REACHED]   = FSM_NO_TRANSITION,
        [FSM_EVENT_CURRENT_THRESHOLD_REACHED]   = { FSM_STATE_FULL,               Fsm_Action_TerminateCharge },
        [FSM_EVENT_BATTERY_FULL]                = { FSM_STATE_FULL,               Fsm_Action_TerminateCharge },
        [FSM_EVENT_OVERTEMPERATURE_DETECTED]    = { FSM_STATE_FAULT,              Fsm_Action_ReportOvertemperature },
        [FSM_EVENT_CHARGER_DISCONNECTED]        = { FSM_STATE_IDLE,               Fsm_Action_StopCharging },
    },
    [FSM_STATE_FULL] = {
        [FSM_EVENT_CHARGER_CONNECTED]           = FSM_NO_TRANSITION,
        [FSM_EVENT_VOLTAGE_THRESHOLD_REACHED]   = FSM_NO_TRANSITION,
        [FSM_EVENT_CURRENT_THRESHOLD_REACHED]   = FSM_NO_TRANSITION,
        [FSM_EVENT_BATTERY_FULL]                = FSM_NO_TRANSITION,
        [FSM_EVENT_OVERTEMPERATURE_DETECTED]    = { FSM_STATE_FAULT,              Fsm_Action_ReportOvertemperature },
        [FSM_EVENT_CHARGER_DISCONNECTED]        = { FSM_STATE_IDLE,               Fsm_Action_StopCharging },
    },
    [FSM_STATE_FAULT] = {
        [FSM_EVENT_CHARGER_CONNECTED]           = FSM_NO_TRANSITION,
        [FSM_EVENT_VOLTAGE_THRESHOLD_REACHED]   = FSM_NO_TRANSITION,
        [FSM_EVENT_CURRENT_THRESHOLD_REACHED]   = FSM_NO_TRANSITION,
        [FSM_EVENT_BATTERY_FULL]                = FSM_NO_TRANSITION,
        [FSM_EVENT_OVERTEMPERATURE_DETECTED]    = FSM_NO_TRANSITION,
        [FSM_EVENT_CHARGER_DISCONNECTED]        = { FSM_STATE_IDLE,               Fsm_Action_ClearFault },
    },
};

static const char *const stateNames[FSM_STATE_COUNT] = {
    "Idle", "TrickleCharge", "ConstantCurrent", "ConstantVoltage", "Full", "Fault"
};

static const char *const eventNames[FSM_EVENT_COUNT] = {
    "ChargerConnected", "VoltageThresholdReached", "CurrentThresholdReached",
    "BatteryFull", "OvertemperatureDetected", "ChargerDisconnected"
};

static void invokeHook(FsmActionFn_t hook)
{
    if (hook != NULL) {
        hook();
    }
}

void Fsm_Init(FsmContext_t *ctx)
{
    ctx->currentState = FSM_STATE_IDLE;
    invokeHook(stateTable[FSM_STATE_IDLE].onEntry);
}

bool Fsm_HandleEvent(FsmContext_t *ctx, FsmEvent_t event)
{
    const FsmTransition_t *transition;
    FsmState_t previous;
    FsmState_t next;

    if (event >= FSM_EVENT_COUNT) {
        return false;
    }

    transition = &transitionTable[ctx->currentState][event];
    if (transition->nextState >= FSM_STATE_COUNT) {
        return false;
    }

    previous = ctx->currentState;
    next = transition->nextState;

    invokeHook(stateTable[previous].onExit);
    invokeHook(transition->action);
    ctx->currentState = next;
    invokeHook(stateTable[next].onEntry);

    return true;
}

FsmState_t Fsm_GetState(const FsmContext_t *ctx)
{
    return ctx->currentState;
}

const char *Fsm_StateName(FsmState_t state)
{
    if (state >= FSM_STATE_COUNT) {
        return "Unknown";
    }
    return stateNames[state];
}

const char *Fsm_EventName(FsmEvent_t event)
{
    if (event >= FSM_EVENT_COUNT) {
        return "Unknown";
    }
    return eventNames[event];
}
