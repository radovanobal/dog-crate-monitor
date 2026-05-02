# ADR 0004: Power Cycles and Sleep-Wake Ownership

## Status

Proposed

## Related Documents

1. [Architecture Overview](../overview.md)
2. [ADR 0001: Screen and Display Ownership Boundaries](0001-screen-display-ownership.md)
3. [ADR 0002: Screen Transition and Render Invalidation Policy](0002-screen-transition-render-invalidation.md)
4. [ADR 0003: Generation-Scoped Render Coalescing](0003-generation-scoped-render-coalescing.md)

## Context

The device is intended to operate both as an interactive e-paper application and as a low-power environmental monitor.

That creates a power-management problem that crosses multiple architectural layers:

1. The display retains its image while the MCU sleeps, so the device can save power between environment reads.
2. Deep sleep on ESP32-S3 behaves like a restart cycle rather than a paused task scheduler.
3. The current runtime model uses long-lived tasks and queues, which fit an always-awake design more naturally than a sleep-cycle design.
4. User interaction, screen changes, BLE behavior, and sensor refresh cadence all influence whether sleep is desirable.
5. External power changes the power budget and should affect automatic sleep policy.

Without an explicit ownership model, several problems are likely:

1. Screens may start making device-level sleep decisions.
2. Display lifecycle and MCU power policy may become entangled.
3. Timer wake, input wake, and cold boot may drift into inconsistent behavior.
4. Background queues may accumulate work that no longer makes sense when preparing for sleep.
5. Charging state and external power may be checked ad hoc in unrelated modules.

## Decision

We will introduce a dedicated power-management policy that owns device sleep eligibility, wake-cause handling, and deep-sleep orchestration.

### 1. Automatic deep sleep is a device-level policy, not a screen policy

Sleep eligibility is owned by a dedicated power-management module.

Screens may expose whether their current mode is sleep-eligible, but screens must not directly put the device into deep sleep.

### 2. Automatic deep sleep is allowed only on battery power

External power presence disables automatic deep sleep.

The primary gate is external power presence rather than charging state alone, because a fully charged battery may stop charging while the device is still externally powered.

### 3. Interactive idle and passive wake cycles are distinct runtime behaviors

The intended model is:

1. While interactive, normal tasks and event handling remain active.
2. After a configurable idle timeout in a sleep-eligible mode, the device may prepare for deep sleep.
3. A timer wake performs a bounded passive cycle: read environment, update the display if needed, then sleep again.
4. An input wake restores interactive behavior instead of immediately returning to sleep.

### 4. Wake cause determines runtime behavior

The system distinguishes at least these wake classes:

1. Cold boot
2. Timer wake
3. User-input wake

These wake causes may share initialization code, but they do not imply identical runtime behavior.

### 5. Display lifecycle remains separate from power policy

The display refresh layer continues to own panel-facing behavior such as display initialization, safe refresh execution, and display sleep or wake operations at the panel level.

The power-management module owns MCU sleep policy and coordinates with the display refresh layer through a defined interface.

### 6. Sleep entry is a controlled transition

Entering deep sleep is not an immediate side effect of an idle timeout.

Before sleep, the power-management module should:

1. Confirm that the current mode is sleep-eligible
2. Confirm that external power is not present
3. Confirm that the idle timeout has expired
4. Avoid interrupting an active display refresh or other critical work
5. Configure wake sources
6. Coordinate any required display or peripheral preparation for sleep

Deep sleep must not be triggered while the panel is in the middle of a refresh sequence.

If power is removed from the MCU-side workflow during an e-paper update, the panel may be left in a clear state, a recovery-required state, or another visually corrupted intermediate state.

For that reason, an in-progress display update is treated as a hard sleep blocker until the refresh layer reports that the panel is safe again.

### 7. Deep sleep is modeled as a new work session, not as task suspension

Timer wake behavior should be treated as a bounded read-evaluate-render-sleep cycle rather than as a resumed background polling loop.

Once sleep preparation starts, queued work from the current awake session should be treated as disposable by default.

The only exception is work that represents an update already in progress on the display path.

If the display layer is already rendering, sleep must be blocked until that render completes or the display layer explicitly reports that the panel is safe.

All other queued work should be dropped rather than carried across the sleep boundary.

### 8. Sleep policy distinguishes manual sleep from passive idle sleep

The power-management module distinguishes at least two deep-sleep entry profiles:

1. Manual sleep, requested explicitly by the user
2. Passive idle sleep, entered automatically after an interactive idle timeout

Manual sleep is intended to behave like a user-selected off state.

When manual sleep is entered, the device should sleep until a user-input wake source fires.

The intended behavior is that any supported button input may wake the device from deep sleep, subject to the hardware wake capabilities and wiring of the board.

Passive idle sleep is intended to support low-power monitoring.

When passive idle sleep is entered, the device should arm both timer wake and input wake so it can perform periodic bounded monitoring cycles while still allowing immediate return to interactive use.

The intended input-wake policy is that any supported button input may wake the device from deep sleep, subject to the hardware wake capabilities and wiring of the board.

### 9. Sleep preparation blocks background intake, but user input remains authoritative

When the interactive idle timeout expires, the power-management module enters a sleep-preparation phase rather than sleeping immediately.

During sleep preparation:

1. New background work should be blocked, ignored, or dropped
2. Already accepted in-flight critical work may finish
3. Already queued work may be drained only to the extent needed to reach a display-safe and sleep-safe state

User input is always authoritative.

If user input arrives during sleep preparation, sleep preparation must be canceled, the idle timer must be reset, and the system must return to interactive behavior.

This means sleep preparation is allowed to suppress background producers, but it must not suppress the user path that cancels the transition.

## Runtime Model

The intended runtime behavior is:

1. Cold boot starts the interactive runtime unless a later mode-specific policy says otherwise.
2. User input during interactive runtime resets the idle timer.
3. A manual sleep request enters a manual sleep-preparation path and then deep sleep with input wake enabled.
4. If the device is battery-powered, in a sleep-eligible mode, and idle past the configured timeout, passive sleep preparation begins.
5. Passive sleep preparation blocks new background intake, allows the current critical path to reach a sleep-safe state, and is canceled immediately by new user input.
6. Timer wake runs a bounded passive monitoring cycle: read environment, evaluate whether display work is needed, perform any required render, and return to deep sleep.
7. Input wake from any supported button returns to interactive behavior and resets the idle timer.
8. External power disables automatic deep sleep even if the idle timeout expires.

## Consequences

### Positive

1. Power behavior becomes owned by one architectural boundary instead of being scattered across screens and services.
2. The e-paper display's persistent image can be used to reduce battery drain between environment reads.
3. Timer wake and input wake become easier to reason about because they follow different, intentional paths.
4. External power behavior becomes consistent for charging, bench use, and debugging.
5. The architecture remains compatible with both passive monitoring and interactive UI sessions.
6. Sleep preparation remains responsive because user input cancels the transition rather than being ignored until sleep completes.

### Tradeoffs

1. Deep sleep requires treating wake as a new session rather than a continuation of all running tasks.
2. Some current task and queue behavior will need reshaping to fit the passive-cycle model cleanly.
3. Retained state and wake-cause interpretation become explicit design concerns.
4. BLE behavior may need a separate policy if continuous availability is required while the device is otherwise trying to save power.
5. Sleep entry may need to wait for long e-paper refreshes to complete before power can be reduced safely.
6. Background producers may need explicit gating so passive sleep preparation can stop new nonessential work from entering the pipeline.

## Rules Derived From This Decision

1. A screen must never directly own MCU deep-sleep entry.
2. Automatic deep sleep is permitted only when the device is battery-powered.
3. External power presence disables automatic deep sleep, regardless of whether the battery is actively charging.
4. Automatic deep sleep also requires a sleep-eligible mode and an expired idle timeout.
5. Timer wake and user-input wake are different runtime entry paths and may perform different work.
6. The display refresh layer owns panel lifecycle; the power-management module owns MCU sleep policy.
7. Deep sleep entry must avoid interrupting active critical work such as an in-progress display refresh.
8. Passive timer-wake operation should prefer a bounded read-evaluate-render-sleep cycle over a persistent polling loop.
9. State required after wake must be explicitly retained or reconstructed; it must not be assumed to survive implicitly in task-local runtime state.
10. An in-progress display refresh is a hard sleep blocker because sleeping mid-refresh can leave the panel in a clear, recovery, or otherwise corrupted visible state.
11. Once sleep preparation starts, queued work is dropped unless it represents a display update already in progress that must finish before sleep.
12. The sleep policy distinguishes manual sleep from passive idle sleep, and those profiles may arm different wake sources.
13. Passive idle sleep should arm both timer wake and input wake.
14. Manual sleep should arm input wake and should not require periodic timer wake.
15. The intended input-wake policy is that any supported button input may wake the device from deep sleep, subject to hardware wake capabilities and board wiring.
16. Sleep preparation may block new background work from entering the pipeline, but it must not block user input from canceling the transition.
17. User input received during sleep preparation cancels sleep preparation and resets the interactive idle timer.

## Implementation Direction

The expected refactor direction is:

1. Introduce a dedicated power-management module
2. Expose shared power-source state such as battery level and external-power presence through shared application state
3. Add explicit sleep-eligibility rules per runtime mode
4. Add wake-cause handling that distinguishes cold boot, timer wake, and input wake
5. Define what render or service work must finish, be discarded, or be rebuilt when preparing for sleep
6. Keep display sleep or wake actions behind the display refresh interface rather than invoking panel control directly from screens
7. Evolve periodic environment update logic toward a bounded passive cycle for timer-wake operation
8. Expose a display-safe-to-sleep signal or equivalent display-busy state so power management can block sleep until the panel update is complete
9. Purge non-display-in-progress queued work when sleep preparation begins instead of carrying it across the sleep boundary
10. Add explicit sleep profiles for manual sleep and passive idle sleep, including different wake-source configuration for each profile
11. Gate background event producers during sleep preparation while keeping the user-input path able to cancel the transition
12. Replace vague queue-empty checks with explicit sleep blockers and a bounded pipeline-drain policy

## Alternatives Considered

### Let the home screen own sleep entry

Rejected because deep sleep is a device-level lifecycle decision rather than a screen-local UI concern.

### Allow automatic deep sleep whenever the device is idle, even on external power

Rejected because external power removes the main battery-life constraint and would make charging and bench-debug behavior less predictable.

### Treat deep sleep as a pause-and-resume mechanism for the existing task graph

Rejected because ESP32 deep sleep behaves more like a restart cycle than suspended multitasking.