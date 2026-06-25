# Battery Charger FSM

A table-driven finite state machine (FSM) written in C that simulates a Li-ion battery charging controller. Structured like real embedded firmware — clean separation of engine, transition table, and hardware action layer.

---

## Overview

This project models the full CC/CV (Constant Current / Constant Voltage) charging algorithm used in real Li-ion battery management ICs (e.g. BQ24195, MAX1898). Instead of driving real GPIO and PWM hardware, all actions are logged via `printf` — making it easy to follow the state machine's behaviour on any machine.

The FSM has **6 states** and **6 events**, with a static 2D transition table mapping every `(state, event)` pair to a `(next state, action function pointer)`.

---

## States

| State | Description |
|---|---|
| `Idle` | Charger output disabled, monitoring battery |
| `TrickleCharge` | 50 mA pre-conditioning for deeply discharged cells |
| `ConstantCurrent` | 1.0 A bulk charge — fills battery to ~80% |
| `ConstantVoltage` | 4.20 V regulation — tapers current to top off safely |
| `Full` | Charge complete, float/termination maintenance |
| `Fault` | Emergency shutdown, fault latch set |

## Events

| Event | Meaning |
|---|---|
| `ChargerConnected` | Charger plugged in |
| `VoltageThresholdReached` | Cell voltage crossed a threshold (used twice: trickle→CC and CC→CV) |
| `CurrentThresholdReached` | Termination current reached — battery full |
| `BatteryFull` | Explicit full signal from fuel gauge or timer |
| `OvertemperatureDetected` | Thermal interrupt — triggers fault from any charging state |
| `ChargerDisconnected` | Charger removed — returns to Idle from any state |

---

## File Structure

| File | Purpose |
|---|---|
| `fsm.h` | Defines state and event enums, FSM data structures, and public function declarations |
| `fsm.c` | Implements the state transition table and FSM dispatch logic |
| `actions.c` | Contains state entry/exit functions and transition action implementations |
| `main.c` | Test driver that simulates the normal charging cycle and fault scenarios |

---

### `fsm.h`
Defines `FsmState_t`, `FsmEvent_t`, and `FsmContext_t`. Declares the four public API functions: `Fsm_Init`, `Fsm_HandleEvent`, `Fsm_GetState`, `Fsm_StateName`, `Fsm_EventName`.

### `fsm.c`
Contains the static `transitionTable[FSM_STATE_COUNT][FSM_EVENT_COUNT]` — the heart of the design. Also contains the `stateTable` of entry/exit hook pointers, and the dispatch engine in `Fsm_HandleEvent` which runs: exit hook → transition action → state change → entry hook.

### `actions.c`
All simulated hardware actions. In target firmware these would drive GPIO, PWM, and ADC comparators. Entry/exit hooks and transition actions are all here, keeping hardware concerns completely separate from the FSM engine.

### `main.c`
Runs two test sequences and prints every state transition with its before/after state:
- **Test 1** — Normal CC/CV charge cycle from cold start to full
- **Test 2** — Overtemperature fault interrupting a charge in progress

---


##  Important Points

**Why table-driven?**
A `switch`/`if-else` FSM grows unmanageable as states and events multiply. The table approach keeps all transition logic in one place — adding a new state means adding one row. The dispatch engine never changes.

**Function pointers for actions**
Entry hooks, exit hooks, and transition actions are all stored as `void (*)(void)` function pointers in the tables. The engine calls them without knowing what they do — hardware concerns stay entirely in `actions.c`.

**Sentinel value**
`FSM_NO_TRANSITION` expands to `{ FSM_STATE_COUNT, NULL }`. Since `FSM_STATE_COUNT` is one past the last valid state, the engine uses `transition->nextState >= FSM_STATE_COUNT` as a fast "no valid transition" check — no separate boolean flag needed.

**Transition sequence**
Every valid state change follows the same four-step order:
1. Exit hook of the current state
2. Transition action
3. State variable update
4. Entry hook of the new state

This ordering guarantees the hardware is always in a consistent state when any hook runs.

---



