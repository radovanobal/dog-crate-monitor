# ADR 0003: Generation-Scoped Render Coalescing

## Status

Proposed

## Related Documents

1. [Architecture Overview](../overview.md)
2. [ADR 0001: Screen and Display Ownership Boundaries](0001-screen-display-ownership.md)
3. [ADR 0002: Screen Transition and Render Invalidation Policy](0002-screen-transition-render-invalidation.md)

## Context

The current architecture separates event handling from rendering.

That is useful, but it creates a problem on a slow e-paper display:

1. Input events can arrive faster than the panel can refresh.
2. Background updates such as time or sensor changes can also enqueue render work.
3. Some render work is partial and region-scoped rather than a full-screen snapshot.
4. Replaying every queued render request in FIFO order increases visible lag.
5. Replacing the render queue with a single latest request is unsafe when requests contain different dirty regions that still need to be painted.

This means the render pipeline needs a coalescing policy rather than a plain append-only queue policy.

## Decision

We will treat queued render work as generation-scoped, mergeable invalidation rather than as a strict replay log.

### 1. Render requests are mergeable only within the same screen generation

If two queued render requests belong to the same active-screen generation, the render pipeline may merge them into one combined render request.

The merged request should represent:

1. The latest screen-local and shared application state available at merge time
2. The union of all still-dirty regions from that generation
3. The newest render content for each region that remains dirty

### 2. Screen generation changes are hard merge boundaries

If a queued render request belongs to a newer screen generation than the current merge accumulator:

1. All older-generation queued render work becomes stale
2. The current merged accumulator is discarded
3. Merging restarts from the newer generation

Queued render work from an older generation must never be merged into a newer generation.

### 3. Input events keep their FIFO semantics

All input events must still be handled in order so screen-local state transitions remain correct.

Example:

1. Three quick rotary-down events should still move menu selection three positions
2. The renderer does not need to display all three intermediate visual states
3. The renderer should display the final state plus any other still-dirty regions for the same generation

### 4. Rendering should prefer the freshest state over replaying historical deltas

Render work should be evaluated from current store data and current screen-local state wherever possible.

Queued render requests are therefore treated as invalidation carriers, not as authoritative historical paint operations that must always be replayed exactly as enqueued.

### 5. Full-screen paints remain non-mergeable boundaries

If a full-screen paint is queued for a generation:

1. Earlier partial render work from that same generation may be subsumed by the full-screen paint
2. Later partial requests from that same generation may be merged only if the implementation explicitly supports post-full invalidation accumulation
3. No implementation should merge two render requests in a way that weakens the guarantee that the first paint of a generation is full-screen

## Queue and Merge Model

The intended render-task behavior is:

1. Wait for the first queued render request
2. Use its generation as the current merge generation
3. Drain additional queued render requests while they are immediately available
4. Merge requests from the same generation into one accumulated render request
5. If a newer generation is encountered, discard the old accumulator and restart from the new generation
6. Ignore or discard any request older than the current merge generation
7. Render the final accumulated request once the queue is empty for that pass

This model coalesces rapid input-driven changes and background updates without allowing stale work from an old screen generation to delay the current screen.

## Consequences

### Positive

1. Rapid input no longer creates a long backlog of stale intermediate paints.
2. Partial sensor, time, and status updates from the same generation can still be preserved.
3. Screen transitions remain authoritative because generation changes invalidate older queued work.
4. The renderer spends more time drawing relevant final state and less time replaying obsolete intermediate state.
5. The policy fits e-paper constraints better than pure FIFO rendering.

### Tradeoffs

1. Render requests need a defined merge contract.
2. Dirty-region accumulation becomes part of render-pipeline correctness.
3. The render task becomes more complex than a plain dequeue-and-render loop.
4. Some intermediate visual states will no longer be shown, even though their input events were processed.

## Rules Derived From This Decision

1. Input events are processed in FIFO order even when renders are coalesced.
2. Render requests from the same screen generation may be merged.
3. Render requests from different screen generations must never be merged together.
4. A newer screen generation invalidates all queued render work from older generations.
5. Render requests should preserve the union of dirty regions that remain relevant within the same generation.
6. Render coalescing should prefer the freshest state for a region rather than replaying every historical region update.
7. The first visible paint of a new generation remains a full refresh.
8. Full-screen paint requests are merge boundaries unless later implementation explicitly defines a safe post-full accumulation rule.

## Implementation Direction

The expected refactor direction is:

1. Define a merge contract for `DisplayRequest` and `DisplayRenderPlan`
2. Distinguish replaceable request fields from unioned dirty-region fields
3. Drain immediately available queued render requests inside the render task before painting
4. Restart the merge accumulator when a newer generation is encountered
5. Discard stale older-generation requests without rendering them
6. Keep render-time generation validation as a defensive fallback
7. Evolve screens and the display refresh layer toward render plans that can be rebuilt from fresh state while preserving relevant invalidation

## Alternatives Considered

### Keep pure FIFO rendering

Rejected because it replays stale intermediate visual states and increases perceived latency on a slow e-paper panel.

### Keep only the latest queued render request

Rejected as a general policy because some render requests carry different dirty regions that still need to be preserved within the same generation.

### Flush all non-input render work whenever input arrives

Rejected because background updates such as time and sensor changes should remain renderable when they are still relevant to the current screen generation.