# Housekeeping

Running log of code-cleanup / dead-code-removal tasks for the simulator. Each task is
timestamped (date added) so we can track how long it has been outstanding. Mark tasks
`Done` (with a completion date) rather than deleting them, so the history stays visible.

Status legend: `Open` · `In progress` · `Done`

---

## HK-1 — Remove `hit_where` from PACKET

- **Added:** 2026-06-18
- **Status:** Open

`PACKET::hit_where` (and the `hit_where_t` enum) was added for a one-off analysis. It is
half-baked, populated only inside `cache.cc`, and not consumed anywhere meaningful. Drop
the whole thing on inspection.

- Field + enum + init: `inc/block.h` (`hit_where_t` enum, `hit_where_t hit_where;`, and the
  `hit_where = hit_where_t::INV;` constructor init).
- Populations: all `*.hit_where = ...` assignments in `src/cache.cc`.
- ~39 references across `inc/` + `src/` — verify each is purely write/init (no real reader)
  before removing.

## HK-2 — Remove stale `rob_signal` from PACKET

- **Added:** 2026-06-18
- **Status:** Open

`PACKET::rob_signal` is a stale, already-deprecated flag (commented `// this is deprecated`
in `inc/block.h`). Remove the field and its initializer.

- Declaration + init: `inc/block.h` (`rob_signal,` in the field list; `rob_signal = -1;` in
  the constructor).
- Only ~3 references total — confirm nothing reads it, then drop.
