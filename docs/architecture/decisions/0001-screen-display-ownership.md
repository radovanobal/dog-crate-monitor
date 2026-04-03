# ADR 0001: Screen and Display Ownership Boundaries

## Status

Accepted

## Related Documents

1. [Architecture Overview](../overview.md)
2. [ADR 0002: Screen Transition and Render Invalidation Policy](0002-screen-transition-render-invalidation.md)

## Context

The current application structure is growing from a single-screen prototype toward a multi-screen application.

As more screens are added, several risks become more important:

1. Screen modules can begin to own hardware lifecycle behavior that should outlive any single screen.
2. Core infrastructure can become tightly coupled to specific screens.
3. A single global display state structure can grow until every screen depends on unrelated fields.
4. Partial and full refresh policy can become inconsistent if each screen makes panel-level decisions independently.

This project uses an e-paper display, so refresh strategy and panel lifecycle are important architectural concerns rather than minor rendering details.

## Decision

We will separate ownership across four layers.

### 1. App Store and Dispatcher

Own shared application state and shared event application.

Examples:

1. Environment data
2. Time data
3. Connectivity state
4. Mode state
5. Navigation state that needs to be globally visible

### 2. Screen Manager

Own active screen lifecycle and event routing.

Responsibilities:

1. Select the active screen
2. Activate and deactivate screens
3. Route events to the active screen
4. Force a full refresh when the active screen changes

The screen manager must remain screen-agnostic. It should not assemble screen-specific display content.

### 3. Screens

Own only screen-local UI behavior and render intent.

Responsibilities:

1. Define layout, grid, and regions
2. Define screen-local state
3. Convert shared app state into screen-specific view state
4. Report which render regions became dirty

Screens must not own:

1. Panel wake and sleep decisions
2. Global display buffers
3. Partial versus full refresh policy
4. Shared application state transitions

### 4. Display Refresh Layer

Own the panel-facing refresh system.

Responsibilities:

1. Display initialization and display lifecycle management
2. Buffer ownership
3. Physical output transforms such as rotation and mirroring
4. Dirty-region merge logic
5. Partial versus full refresh decisions
6. Safe execution of display updates

If the device needs mirrored output for viewing in a rear-view mirror, that behavior is treated as a display output policy rather than as a screen concern.

## Consequences

### Positive

1. Adding a new screen becomes cheaper because screens stay behind a generic interface.
2. Panel safety logic stays centralized instead of being scattered across screens.
3. Dirty-region merging can be implemented once and reused by all screens.
4. Shared app state stays separate from local screen UI state.
5. Screen activation behavior becomes predictable.

### Tradeoffs

1. The refactor introduces an extra abstraction layer for display refresh.
2. Some current code will need to move even if behavior stays the same.
3. The screen interface will likely need to grow to support screen-local state and invalidation reporting.

## Rules Derived From This Decision

1. The first render after screen activation is always a full refresh.
2. Screens prepare logical render content and report invalidation; the display refresh layer chooses the panel refresh strategy.
3. Screen deactivation clears screen-local state only.
4. Display sleep and wake behavior is never owned by an individual screen.
5. Shared application state is updated before the active screen derives new render state from it.
6. Screens render in logical coordinates, and the display refresh layer applies any physical output transform.
7. Navigation uses a full-screen replacement model rather than overlays, popups, slide-in menus, or a stacked screen model.
8. Local highlight changes inside the currently active screen may still use partial updates when appropriate.
9. Power management and display lifecycle remain separate concerns and communicate through a defined interface.

## Implementation Direction

The expected refactor direction is:

1. Remove panel lifecycle ownership from individual screens.
2. Introduce a display refresh module that owns buffers and refresh policy.
3. Keep screen manager focused on lifecycle and routing.
4. Evolve screens toward reporting dirty regions and screen-prepared render content instead of returning one global display-state shape.
5. Allow partial render requests from the same active-screen generation to be coalesced until a full-screen paint is queued.
6. Never merge full-screen paint requests with other render requests.
7. Keep power-management orchestration in a dedicated module that coordinates with the display refresh layer through a clear boundary.

## Alternatives Considered

### Let each screen own its own refresh policy

Rejected because it duplicates e-paper refresh logic and makes long-term consistency difficult.

### Keep one global display state structure for all screens

Rejected because it couples unrelated screens together and encourages constant expansion of shared types.

### Let screen manager assemble per-screen display data

Rejected because it makes the manager screen-aware and increases the cost of adding new screens.
