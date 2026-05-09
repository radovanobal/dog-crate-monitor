# ADR 0005: Display Request Queue Ownership and Storage

## Status

Proposed

## Related Documents

1. [Architecture Overview](../overview.md)
2. [ADR 0001: Screen and Display Ownership Boundaries](0001-screen-display-ownership.md)
3. [ADR 0003: Generation-Scoped Render Coalescing](0003-generation-scoped-render-coalescing.md)
4. [ADR 0004: Power Cycles and Sleep-Wake Ownership](0004-power-cycles.md)

## Context

The current render pipeline passes `DisplayRequest` by value through the display queue and also keeps large render-related values on task stacks.

That creates several architectural problems:

1. `DisplayRequest` contains a full `DisplayRenderPlan` by value.
2. `DisplayRenderPlan` contains multiple `RenderRegionScene` entries by value.
3. `RenderRegionScene` contains multiple `PixelRenderItem` entries by value.
4. `PixelRenderItem` reserves space for its largest union member, including `BoxGridData`.
5. Core-0 tasks therefore reserve large stack frames even when a screen does not use grid render items.
6. The display queue also copies large request payloads on every send and receive.
7. Queue reset on screen-generation changes is simple today only because request storage is embedded directly in the queue item.

This is a poor long-term fit for an ESP32-S3 system with small task stacks and an e-paper render pipeline that is expected to grow.

## Decision

We will move display-request transport from by-value queue items to pointer-based queue ownership backed by fixed request storage.

### 1. The display queue will carry request pointers, not full `DisplayRequest` values

The display queue item type should become a pointer to an owned request object.

The queue is treated as a transport for ownership transfer, not as storage for the render payload itself.

### 2. `DisplayRequest` storage will come from a fixed request pool

Render requests should live in a small fixed pool or equivalent dedicated storage owned by the display-task boundary.

The request object must not be created as a large task-local stack variable in either the UI task or the render task.

### 3. Request ownership must be explicit

The intended ownership flow is:

1. The UI-side producer acquires a free request slot.
2. The UI-side producer fills the request slot.
3. The producer sends a `DisplayRequest *` through the display queue.
4. The render task owns that request pointer while it is merging or rendering it.
5. The render task releases the request slot back to the pool when it is finished.

No request slot may be reused while it is still in flight.

Task manager is the sole owner of slot-state transitions and is the only module allowed to return a request slot to `FREE`.

### 4. `AppEvent` remains by value

This decision applies only to the display-request path.

`AppEvent` remains small enough to pass by value and does not need the same ownership machinery.

### 5. Screen-generation changes remain authoritative invalidation boundaries

Generation changes still invalidate older render work as defined in [ADR 0003](0003-generation-scoped-render-coalescing.md).

When the active screen generation changes, any queued request pointers from an older generation must be reclaimed or invalidated safely.

Queue reset is no longer just a queue operation. It becomes part of request-lifetime management.

### 6. Request reclamation must be safe under queue purge and sleep preparation

Any path that discards queued render work must also return or invalidate the corresponding request slots safely.

This includes at least:

1. Screen-generation change handling
2. Sleep-preparation queue purge behavior
3. Any future render-cancellation path

### 7. The long-term goal is to keep large render payloads out of task stacks entirely

Pointer transport is not just a queue optimization.

It is part of a larger rule:

1. Large render payloads should not live on task stacks.
2. Large render payloads should not be copied through queues by value.
3. Render-request lifetime should be explicit and inspectable.

## Consequences

### Positive

1. Core-0 task stack pressure is reduced substantially.
2. Queue send and receive no longer copy large render payloads.
3. Render-request ownership becomes explicit instead of being hidden inside queue copies.
4. The render path is better prepared for larger scenes, more regions, and richer item types.
5. Future debugging becomes easier because request lifetime can be logged and validated directly.

### Tradeoffs

1. A fixed request pool introduces explicit lifetime management.
2. Queue reset and queue purge logic become more complex because storage reclamation must also happen.
3. Bugs can shift from stack overflow toward ownership mistakes such as stale-slot reuse or double release.
4. The render-task boundary becomes more explicit and therefore slightly more verbose.

## Rules Derived From This Decision

1. The display queue must never own the render payload by value.
2. A `DisplayRequest` in flight must live in dedicated storage, not task-local stack memory.
3. A queued `DisplayRequest *` must remain valid until the render task releases it.
4. A request slot must not be reused until ownership is returned to the pool.
5. Queue purge or reset must reclaim or invalidate the corresponding request slots.
6. A request from an older screen generation must never survive as valid work after a newer generation becomes active.
7. `AppEvent` and display-request transport are intentionally different ownership models.

## Implementation Direction

The expected refactor direction is:

1. Change the display queue item type from `DisplayRequest` to `DisplayRequest *`
2. Introduce a small fixed request pool for render work
3. Move large request construction out of UI-task stack variables
4. Move render-task merge buffers out of render-task stack variables
5. Add explicit acquire, release, and invalidation operations for request slots
6. Teach screen-generation change handling how to reclaim queued request pointers safely
7. Teach any future sleep-preparation purge path how to reclaim queued request pointers safely
8. Add diagnostics for request-slot lifecycle, such as slot state, generation, and owner

## Alternatives Considered

### Keep by-value queue transport and only increase task stack sizes

Rejected because it treats the symptom rather than the ownership problem and still leaves large queue-copy overhead in place.

### Keep by-value queue transport but move only some locals to static storage

Rejected as a useful short-term diagnostic tactic but not as the architectural direction, because it still keeps the queue payload large and still hides ownership.

### Replace the queue with a single latest-request mailbox immediately

Deferred.

This may still become a good fit later, but the current design already has generation-scoped coalescing semantics. A pointer-based queue with explicit ownership is the safer next step because it solves the storage problem without forcing a larger scheduling-model change at the same time.