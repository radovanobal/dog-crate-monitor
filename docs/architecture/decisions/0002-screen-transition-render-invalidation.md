# ADR 0002: Screen Transition and Render Invalidation Policy

## Status

Accepted

## Related Documents

1. [Architecture Overview](../overview.md)
2. [ADR 0001: Screen and Display Ownership Boundaries](0001-screen-display-ownership.md)

## Context

The application uses separate tasks and queues for event handling and rendering. That means render requests can remain queued while the active screen changes.

Without an explicit transition policy, several problems can appear:

1. A render request created by the previous screen can execute after a screen switch.
2. The render task may need to compare queued work against the current active screen to avoid drawing stale output.
3. The same screen ID may become active again later, making screen ID alone a weak identifier for queued render work.
4. Old partial updates can delay or interfere with the first correct render of the newly active screen.

This is especially important for an e-paper display because unnecessary refreshes cost time and power, and stale partial updates can produce confusing visual results.

## Decision

We will treat screen transitions as render-invalidation boundaries.

When a screen transition occurs:

1. Pending render work from the previous screen instance becomes stale.
2. Stale render work should be purged rather than drained by default.
3. The newly active screen starts a new render generation.
4. The first render for the new generation is always a full refresh.
5. Render-time validation remains in place as a defensive fallback.
6. Stale queued work should be explicitly removed where practical rather than only being skipped later.
7. Generation invalidation applies to all work scoped to a screen generation, not only render requests.
8. No special-case drain policy is planned for future transitions.

## Transition Model

The intended transition flow is:

1. A screen change is requested.
2. The screen manager starts a new active-screen generation.
3. Pending render work from older generations is invalidated.
4. The previous screen is deactivated.
5. The new screen is activated.
6. A full render is requested for the new screen generation.
7. Normal partial-update behavior resumes after the first full render completes.

## Generation-Based Render Identity

Render requests should carry a generation or epoch value that identifies the active-screen session that produced them.

This is preferred over using screen ID alone because the same screen may be activated more than once during the app's lifetime.

Example:

1. Home screen active, generation 12
2. Menu screen active, generation 13
3. Home screen active again, generation 14

In that model, a stale render request from generation 12 is rejected even though its screen ID matches the current screen ID.

## Queue Policy

The default queue policy for screen transitions is purge rather than drain.

### Purge means

1. Stale queued render requests are discarded.
2. The screen switch is allowed to complete without waiting for the old screen's pending renders.
3. The new screen becomes authoritative immediately.

### Drain means

1. Old queued render requests are rendered before the screen switch completes.
2. Screen switching latency increases.
3. The system may spend time updating a screen the user is leaving.

Drain is rejected as the default policy because it adds latency and wasted work without improving the correctness of the final screen state.

## Consequences

### Positive

1. Screen switches become easier to reason about.
2. Stale render work is invalidated explicitly instead of only being filtered late.
3. The first visible state of a new screen becomes deterministic.
4. The queue and render pipeline remain safe even when returning to the same screen later.
5. E-paper refresh cost is reduced because stale work is not rendered unnecessarily.

### Tradeoffs

1. Render requests need an additional generation field or equivalent transition identity.
2. The render queue or render pipeline needs explicit invalidation handling.
3. Some queued work will be discarded instead of completed.

## Rules Derived From This Decision

1. A screen transition invalidates pending render work from older screen generations.
2. A render request must be associated with the generation that created it.
3. Render requests from stale generations must never be rendered.
4. The first render of a newly activated screen generation is always a full refresh.
5. Render-time validation remains in place even if the queue is purged during transitions.
6. Screen ID checks may remain as a guard, but generation checks are the stronger correctness rule.
7. Stale queued work should be explicitly removed where possible instead of relying only on lazy rejection when dequeued.
8. Generation invalidation applies to all screen-generation-scoped work.
9. Draining old screen work is not part of the default transition model.

## Implementation Direction

The expected refactor direction is:

1. Add a generation or epoch value to render requests.
2. Make screen manager increment or replace that generation on screen activation.
3. Give the render pipeline a way to purge or invalidate stale queued work.
4. Keep render-time validation as a final defensive check.
5. Force a full refresh for the first render in a new generation.
6. Prefer explicit queue cleanup on transition while keeping lazy stale-request rejection as a fallback.
7. Extend generation-aware invalidation to any future queued work that belongs to a specific active-screen lifetime.

## Alternatives Considered

### Use screen ID matching only

Rejected because the same screen can be activated multiple times, so screen ID alone does not uniquely identify a render request's lifetime.

### Drain all queued work before switching screens

Rejected as the default policy because it increases latency and may spend time refreshing output for a screen that is no longer relevant.

### Keep only a late render-time rejection check

Rejected as the primary strategy because it detects stale work too late and leaves queue behavior underdefined. It remains useful as a defensive fallback.