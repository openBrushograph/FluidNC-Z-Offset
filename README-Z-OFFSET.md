# FluidNC Firmware Modification: Realtime Z-Babystepping

This document details the changes made to the `FluidNC` source code to implement realtime, planner-bypassing Z-axis nudging.

## Overview
We intercepted the command stream to recognize two new custom characters (`0xB0` and `0xB1`). These triggers inject a "babystep" event into the protocol loop, which then updates a counter. The core stepping interrupt (ISR) monitors this counter and injects extra step pulses to the Z-axis whenever it is safe to do so.

## 1. New Commands Defined
**File:** `FluidNC/src/RealtimeCmd.h`
**Change:** Added two entries to the `Cmd` enum.
```cpp
enum class Cmd : uint8_t {
    // ... existing commands ...
    CoolantMistOvrToggle  = 0xA1,
    BabystepZUp           = 0xB0, // New: Extended ASCII 176 (Moves Z +0.1mm)
    BabystepZDown         = 0xB1, // New: Extended ASCII 177 (Moves Z -0.1mm)
};
```

## 2. Command Parsing
**File:** `FluidNC/src/RealtimeCmd.cpp`
**Change:** Added dispatch cases for the new commands in `execute_realtime_command`.
```cpp
// ... inside switch(command) ...
case Cmd::BabystepZUp:
    protocol_send_event(&babystepEvent, (void*)1);
    break;
case Cmd::BabystepZDown:
    protocol_send_event(&babystepEvent, (void*)-1);
    break;
```

## 3. Protocol Event Handling
**File:** `FluidNC/src/Protocol.h`
**Change:** Declared the new event.
```cpp
extern const ArgEvent babystepEvent;
```

**File:** `FluidNC/src/Protocol.cpp`
**Change:** Implemented the handler function.
```cpp
static void protocol_do_babystep(void* arg) {
    // Only allow when Idle, Cycle, Jog, or Hold (safe states)
    if (state_is(State::Cycle) || state_is(State::Jog) || state_is(State::Idle) || state_is(State::Hold)) {
        int direction = int(arg);
        // Call the stepper subsystem to register the request
        Stepper::babystep(Z_AXIS, direction > 0);
    }
}
// ...
const ArgEvent babystepEvent { protocol_do_babystep };
```

## 4. Stepper Implementation (The Core)
**File:** `FluidNC/src/Stepper.h`
**Change:** Added function declaration.
```cpp
namespace Stepper {
    // ...
    void babystep(int axis, bool direction);
}
```

**File:** `FluidNC/src/Stepper.cpp`
**Change 1:** Added a volatile accumulator to track pending steps.
```cpp
static volatile int32_t babystep_accumulator[MAX_N_AXIS];

void Stepper::babystep(int axis, bool direction) {
    if (axis < MAX_N_AXIS) {
        if (direction) babystep_accumulator[axis]++;
        else babystep_accumulator[axis]--;
    }
}
```

**Change 2 (Critical):** Injected pulse logic into `pulse_func()`.
Normally, this function only executes pre-calculated "segments" from the planner. We added a block at the *end* of the function (after standard steps are processed) to check for babysteps:
```cpp
// [ANTI-GRAVITY] BABYSTEP INJECTION
int32_t z_acc = babystep_accumulator[Z_AXIS];
if (z_acc != 0) {
    // Safety: Only step if the planner isn't ALREADY stepping Z this cycle
    if (!bitnum_is_true(st.step_outbits, Z_AXIS)) {
         set_bitnum(st.step_outbits, Z_AXIS); // Force Z Step

         if (z_acc > 0) {
             clear_bitnum(st.dir_outbits, Z_AXIS); // Set Direction UP
             babystep_accumulator[Z_AXIS]--;       // Decrement pending
         } else {
             set_bitnum(st.dir_outbits, Z_AXIS);   // Set Direction DOWN
             babystep_accumulator[Z_AXIS]++;       // Decrement pending
         }
    }
}
```
