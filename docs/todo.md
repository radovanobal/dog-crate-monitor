# ADR 0001 Checklist

## Now

- [x] Step 1: lock the screen-to-display contract
  Define the replacement for ScreenRenderResult in [main/screen_types.h#L8](../main/screen_types.h#L8). The current contract only supports isRenderRequired plus DisplayRenderPlan, while [ADR 0001](architecture/decisions/0001-screen-display-ownership.md) expects screens to report what changed instead of selecting panel behavior.
- [x] Step 2: reduce the screen manager to lifecycle and routing only
  Remove refresh-policy decisions from [main/screen_manager.c#L18](../main/screen_manager.c#L18) and [main/screen_manager.c#L34](../main/screen_manager.c#L34). Keep the manager focused on responsibilities like those in [main/screen_manager.c#L57](../main/screen_manager.c#L57), [main/screen_manager.c#L74](../main/screen_manager.c#L74), and [main/screen_manager.c#L80](../main/screen_manager.c#L80).
- [x] Step 3: decide how screen transitions influence refresh planning
  Decide whether the "first render after generation change must be full" rule stays as a transition flag in [main/screen_manager.c#L18](../main/screen_manager.c#L18), or whether the screen manager should only report transition intent and let the display layer make the final refresh decision.
- [x] Step 5: add local screen state for diffing
  The home screen in [main/screens/home_screen.c#L74](../main/screens/home_screen.c#L74) currently derives everything fresh on every evaluation. Add real screen-local view state so the screen can compare previous and current values.
- [x] Step 6: convert the home screen render path to invalidation-driven updates
  Change evaluateDisplay() in [main/screens/home_screen.c#L74](../main/screens/home_screen.c#L74) from "always rebuild everything" to "derive view state, compare, mark dirty".
- [x] Step 7: separate derivation from rendering output
  Split the home screen logic so text and layout derivation is separate from render-item emission. That will make it easier to redraw only dirty regions.
- [x] Step 8: clean up the render queue payload boundary
  Keep the queue payload item-only, and use explicit `CLEAR` render items to represent removal so invalidation does not need separate dirty-region metadata.

## Next

- [ ] Decide the invalidation format
  Pick one:
  - [ ] Region IDs only
  - [ ] Pixel rectangles only
  - [ ] Region IDs plus pixel rectangles
- [ ] Decide whether screens return:
  - [ ] Render items for only dirty regions
  - [ ] A full logical scene plus dirty metadata
  This is the main unresolved interface decision.
- [x] Keep the partial-refresh count policy in one place only
  The counter is now in [main/display_controller.c#L18](../main/display_controller.c#L18), which is better, but the boundary is still not fully clean because the screen manager is still passing an explicit paint type.
- [ ] Define the "force full refresh" rules in the display layer
  Examples to decide:
  - [ ] Screen activation
  - [ ] Dirty area threshold
  - [ ] Dirty region count threshold
  - [ ] Partial refresh count threshold
  - [ ] Safety fallback conditions
- [ ] Decide whether current "partial" behavior is acceptable as an interim step
  The implementation still targets the full panel in [main/display_controller.c#L159](../main/display_controller.c#L159) and [main/display_controller.c#L174](../main/display_controller.c#L174).
- [ ] Decide what "partial update" means in this project right now
  At the moment it means partial painting into the buffer, but not true region-limited panel I/O.
- [x] Document whether true region-based panel updates are blocked by the driver, deferred intentionally, or still under investigation
  For now, treat this as blocked by current driver support and defer the larger ADR 0001 refresh-layer completion until that changes or you choose a different display-update strategy.
- [ ] Decide what should survive deactivation
  [ADR 0001](architecture/decisions/0001-screen-display-ownership.md) says deactivation clears screen-local state only, but that policy is not encoded yet.
- [ ] Give the menu screen a real local state model or explicitly mark it as placeholder
  The current menu screen in [main/screens/menu_screen.c](../main/screens/menu_screen.c) returns isRenderRequired = true with an empty plan, which is likely transitional.
- [ ] Keep the current region model as the basis for invalidation
  The home screen already has named regions in [main/screens/home_screen.c#L17](../main/screens/home_screen.c#L17).
- [ ] Decide whether region-level invalidation is enough for the home screen, or whether you want finer-grained pixel rectangles later
- [x] Keep generation-based stale render rejection
  This is already in good shape in [main/task_manager.c#L95](../main/task_manager.c#L95).
- [x] Keep queue purge on screen-generation changes
  This is already implemented in [main/task_manager.c#L52](../main/task_manager.c#L52).
- [ ] Decide whether dirty-region coalescing should happen:
  - [ ] Only within one render request
  - [ ] Across queued requests from the same screen generation
  This is still open from the [architecture overview](architecture/overview.md) and matters for the display layer design.

## Later

- [ ] Revisit Step 4: turn the display controller into the real refresh layer
  Defer the full ADR 0001 refresh-layer implementation until the driver supports region-limited updates, or until you intentionally design around that limitation with a different buffer and coalescing strategy.
- [ ] Decide whether the hardcoded screen switch in [main/screen_manager.c#L89](../main/screen_manager.c#L89) is acceptable for now
- [ ] If not, introduce a registry table for screens
  That is not the first blocker, but it is still open if you want cheap future screen additions.
- [ ] Remove screen-count assumptions from input navigation
  The `(activeScreen + 1) % 2` toggle in [main/screen_manager.c#L113](../main/screen_manager.c#L113) is a temporary shortcut, not a scalable navigation model.
- [ ] Define the boundary between power management and display lifecycle
  [ADR 0001](architecture/decisions/0001-screen-display-ownership.md) explicitly keeps them separate, but the code has no dedicated interface for that yet.
- [ ] Decide who owns display wake/sleep requests
  Right now initialization is direct in [main/main.c#L21](../main/main.c#L21), but there is no long-term lifecycle API.
- [ ] Decide whether the display layer should expose states like:
  - [ ] Initialized
  - [ ] Awake
  - [ ] Sleeping
  - [ ] Busy refreshing
- [ ] Decide what should happen to queued render work when the display is sleeping or preparing for deep sleep
- [ ] Remove or repurpose the unused DisplayState type in [main/display_types.h#L33](../main/display_types.h#L33)
  That type reflects the older "global display state" approach that ADR 0001 moved away from.
- [ ] Re-check whether DisplayPaintType remains part of the public screen-facing contract after the refactor
  If the display layer owns refresh policy, this type may need to move inward or be replaced.
- [ ] Reconcile ADR 0001 with the architecture overview on navigation/open questions
  [ADR 0001](architecture/decisions/0001-screen-display-ownership.md) says full-screen replacement only, while the [architecture overview](architecture/overview.md) still lists overlays/modal/back-stack as open.
- [ ] Add one short implementation-status note to the architecture docs
  Useful items to record:
  - [ ] Generation-based transition invalidation is implemented
  - [ ] Screen-local invalidation is not implemented yet
  - [ ] True dirty-region merge policy is not implemented yet
  - [ ] Partial panel update is still provisional/full-panel scoped

## Suggested Order

- [ ] Now: finalize the screen-to-display contract
- [x] Now: move refresh planning fully into the display layer
- [x] Now: convert the home screen to local-state diffing and dirty reporting
- [ ] Next: clean up queue payloads and display request types
- [ ] Later: define the power/display lifecycle API
- [ ] Later: remove dead transitional types
- [ ] Later: reconcile the docs

## Highest-Priority Checklist

If you want the shortest actionable subset, I would treat these as the immediate next items:

- [ ] Define the new ScreenRenderResult replacement
- [x] Remove paint-type choice from the screen manager
- [x] Add dirty/invalidation reporting to the home screen
- [ ] Decide whether coalescing happens within one render request or across queued requests
- [ ] Remove or replace the unused global-style DisplayState

## Possible Next Formats

- [ ] A phased implementation checklist with "done when" criteria
- [ ] A checklist grouped by specific files you'll touch
- [ ] A proposed interface sketch in chat for the new screen/display boundary