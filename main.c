/**
 * @file main.c
 * @brief Test driver — feeds simulated events through the charging FSM.
 */

#include "fsm.h"

#include <stdio.h>

/**
 * @brief Inject one event and print the outcome.
 */
static void injectEvent(FsmContext_t *ctx, FsmEvent_t event)
{
    bool accepted;

    printf("\n>> Event: %s  (state before: %s)\n",
           Fsm_EventName(event), Fsm_StateName(Fsm_GetState(ctx)));

    accepted = Fsm_HandleEvent(ctx, event);

    if (accepted) {
        printf(">> Transition complete — now in: %s\n", Fsm_StateName(Fsm_GetState(ctx)));
    } else {
        printf(">> Event ignored in state %s\n", Fsm_StateName(Fsm_GetState(ctx)));
    }
}

/**
 * @brief Simulate a complete CC/CV charge cycle from cold start.
 */
static void runNormalChargeCycle(void)
{
    FsmContext_t ctx;

    printf("\n========================================\n");
    printf(" TEST 1: Normal charge cycle (CC/CV)\n");
    printf("========================================\n");

    Fsm_Init(&ctx);

    injectEvent(&ctx, FSM_EVENT_CHARGER_CONNECTED);
    injectEvent(&ctx, FSM_EVENT_VOLTAGE_THRESHOLD_REACHED);  /* trickle -> CC */
    injectEvent(&ctx, FSM_EVENT_VOLTAGE_THRESHOLD_REACHED);  /* CC -> CV */
    injectEvent(&ctx, FSM_EVENT_CURRENT_THRESHOLD_REACHED);  /* CV -> Full */
    injectEvent(&ctx, FSM_EVENT_CHARGER_DISCONNECTED);
}

/**
 * @brief Simulate overtemperature interrupting an in-progress charge.
 */
static void runOvertemperatureFault(void)
{
    FsmContext_t ctx;

    printf("\n========================================\n");
    printf(" TEST 2: Overtemperature fault scenario\n");
    printf("========================================\n");

    Fsm_Init(&ctx);

    injectEvent(&ctx, FSM_EVENT_CHARGER_CONNECTED);
    injectEvent(&ctx, FSM_EVENT_VOLTAGE_THRESHOLD_REACHED);  /* trickle -> CC */
    injectEvent(&ctx, FSM_EVENT_OVERTEMPERATURE_DETECTED);   /* CC -> Fault */
    injectEvent(&ctx, FSM_EVENT_CHARGER_DISCONNECTED);     /* Fault -> Idle */
}

int main(void)
{
    printf("Battery Charging Controller FSM — simulation build\n");

    runNormalChargeCycle();
    runOvertemperatureFault();

    printf("\nAll test sequences finished.\n");
    return 0;
}
