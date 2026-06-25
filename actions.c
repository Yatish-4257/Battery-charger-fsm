/**
 * @file actions.c
 * @brief State entry/exit hooks and transition action implementations.
 *
 * In target firmware these would drive GPIO, PWM, and ADC comparators.
 * Here they log simulated hardware actions via printf.
 */

#include <stdio.h>

/* -------------------------------------------------------------------------- */
/* State entry hooks                                                          */
/* -------------------------------------------------------------------------- */

void Fsm_OnEnter_Idle(void)
{
    printf("  [HW] Charger output disabled, monitoring battery\n");
}

void Fsm_OnEnter_TrickleCharge(void)
{
    printf("  [HW] Trickle charge enabled (50 mA pre-conditioning)\n");
}

void Fsm_OnEnter_ConstantCurrent(void)
{
    printf("  [HW] Constant-current mode: 1.0 A bulk charge\n");
}

void Fsm_OnEnter_ConstantVoltage(void)
{
    printf("  [HW] Constant-voltage mode: 4.20 V regulation\n");
}

void Fsm_OnEnter_Full(void)
{
    printf("  [HW] Charge complete — maintaining float / termination\n");
}

void Fsm_OnEnter_Fault(void)
{
    printf("  [HW] FAULT: charger disabled, fault latch set\n");
}

/* -------------------------------------------------------------------------- */
/* State exit hooks                                                           */
/* -------------------------------------------------------------------------- */

void Fsm_OnExit_Idle(void)           { printf("  [HW] Leaving idle — arming charger\n"); }
void Fsm_OnExit_TrickleCharge(void)  { printf("  [HW] Trickle charge stopped\n"); }
void Fsm_OnExit_ConstantCurrent(void){ printf("  [HW] CC phase ended\n"); }
void Fsm_OnExit_ConstantVoltage(void) { printf("  [HW] CV phase ended\n"); }
void Fsm_OnExit_Full(void)           { printf("  [HW] Ending maintenance charge\n"); }
void Fsm_OnExit_Fault(void)          { printf("  [HW] Clearing fault latch\n"); }

/* -------------------------------------------------------------------------- */
/* Transition actions (run between exit and entry during a transition)        */
/* -------------------------------------------------------------------------- */

void Fsm_Action_StartTrickleCharge(void)
{
    printf("  [ACT] Charger connected — starting pre-charge sequence\n");
}

void Fsm_Action_BeginConstantCurrent(void)
{
    printf("  [ACT] Minimum cell voltage reached — switching to bulk CC\n");
}

void Fsm_Action_BeginConstantVoltage(void)
{
    printf("  [ACT] Pack voltage at CV setpoint — tapering current\n");
}

void Fsm_Action_TerminateCharge(void)
{
    printf("  [ACT] Termination current met — charge cycle complete\n");
}

void Fsm_Action_StopCharging(void)
{
    printf("  [ACT] Charger disconnected — shutting down power stage\n");
}

void Fsm_Action_ReportOvertemperature(void)
{
    printf("  [ACT] OVERTEMP interrupt — emergency shutdown initiated\n");
}

void Fsm_Action_ClearFault(void)
{
    printf("  [ACT] Fault cleared after charger removal — safe to reconnect\n");
}
