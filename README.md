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

```
battery-charger-fsm/
├── fsm.h        # Enums, structs, and public function declarations
├── fsm.c        # Transition table + FSM dispatch engine
├── actions.c    # State entry/exit hooks and transition action functions
└── main.c       # Test driver — simulates normal cycle and fault scenario
```

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

## Building and Running

No external libraries required. Any C99-compatible compiler works.

**GCC:**
```bash
gcc -std=c99 -Wall -Wextra -o charger_fsm main.c fsm.c actions.c
./charger_fsm
```

**Clang:**
```bash
clang -std=c99 -Wall -Wextra -o charger_fsm main.c fsm.c actions.c
./charger_fsm
```

**MSVC (Windows):**
```cmd
cl main.c fsm.c actions.c /Fe:charger_fsm.exe
charger_fsm.exe
```

---

## Sample Output

```
Battery Charging Controller FSM — simulation build

========================================
 TEST 1: Normal charge cycle (CC/CV)
========================================
  [HW] Charger output disabled, monitoring battery

>> Event: ChargerConnected  (state before: Idle)
  [HW] Leaving idle — arming charger
  [ACT] Charger connected — starting pre-charge sequence
  [HW] Trickle charge enabled (50 mA pre-conditioning)
>> Transition complete — now in: TrickleCharge

>> Event: VoltageThresholdReached  (state before: TrickleCharge)
  [HW] Trickle charge stopped
  [ACT] Minimum cell voltage reached — switching to bulk CC
  [HW] Constant-current mode: 1.0 A bulk charge
>> Transition complete — now in: ConstantCurrent

>> Event: VoltageThresholdReached  (state before: ConstantCurrent)
  [HW] CC phase ended
  [ACT] Pack voltage at CV setpoint — tapering current
  [HW] Constant-voltage mode: 4.20 V regulation
>> Transition complete — now in: ConstantVoltage

>> Event: CurrentThresholdReached  (state before: ConstantVoltage)
  [HW] CV phase ended
  [ACT] Termination current met — charge cycle complete
  [HW] Charge complete — maintaining float / termination
>> Transition complete — now in: Full

>> Event: ChargerDisconnected  (state before: Full)
  [HW] Ending maintenance charge
  [ACT] Charger disconnected — shutting down power stage
  [HW] Charger output disabled, monitoring battery
>> Transition complete — now in: Idle

========================================
 TEST 2: Overtemperature fault scenario
========================================
...
  [ACT] OVERTEMP interrupt — emergency shutdown initiated
  [HW] FAULT: charger disabled, fault latch set
>> Transition complete — now in: Fault
```

---

## Design Notes

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

## Extending the FSM

To add a new state (e.g. `FSM_STATE_BALANCING`):
1. Add the enum value to `FsmState_t` in `fsm.h` (before `FSM_STATE_COUNT`)
2. Add entry/exit functions in `actions.c` and forward-declare them in `fsm.c`
3. Add a row to `stateTable[]` in `fsm.c`
4. Add a row to `transitionTable[]` in `fsm.c` covering all events
5. Update any existing rows that should transition into the new state

---

## License

MIT — free to use, modify, and incorporate into your own firmware projects.
