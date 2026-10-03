# Urbania — Plan to resolve all 18 open issues

Status: **plan only** (no code changed).
Base revision: `da5bbe4` on `main`, in sync with `origin/main`.
Open PRs: **0**. Open issues: **18** (`#1`–`#18`).

This plan is written to be executed top-to-bottom. It states, for every issue, what
the code actually does today (verified by reading the tree at `da5bbe4`), whether it
is already fixed, the exact change, the regression test, and the acceptance
criterion. Nothing in this plan relies on assumptions — every claim below is backed
by a file:line read from the current tree.

---

## 1. Ground truth at HEAD

Two commits already landed after the issues were filed (`2026-10-01T18:29–18:30`):

- `ecc0453` "fix: bus loop routes, save ID cap, and treasury overflow" (#1, #2, #3)
- `da5bbe4` "fix: route cache invalidation, traffic churn, and route validation" (#6, #7, #8, #11)

Those commits **did not close the issues**. Verified status in the current tree:

| # | Title | Verified state at `da5bbe4` | Evidence |
|---|-------|------------------------------|----------|
| 1 | Bus dies on loop routes | **FIXED** | `Bus.cpp:132-141` skips degenerate legs; `Bus.cpp:145` rejects `newPath.size() < 2` |
| 2 | Save rejected after 100k cumulative IDs | **FIXED** | `SaveSystem.cpp:486-491` now only requires `nextCitizenId >= 1`; `CitizenManager.cpp:103-120` floors `nextId` with INT_MAX guard |
| 3 | Signed overflow wipes treasury | **FIXED** (residual: money saturates at `INT_MAX`) | `Economy.cpp:264-268` 64-bit clamp; `Economy.cpp:196` restore clamps both ends |
| 6 | New vehicle per trip forever | **FIXED** | `Vehicle.h:30` adds `direction`; `Traffic.cpp:170-209` shuttles instead of retiring |
| 7 | Route cache keyed on node count | **FIXED** | `RoadNetwork.h:38` + `RoadNetwork.cpp` monotonic `revision()`; `CommuteSystem.cpp:30,40` keys on it; `CommuteSystem.cpp:79-97` revalidates each path |
| 8 | `rebuildAfterLoad` skips `CommuteSystem` | **FIXED** | `Simulation.cpp:147` `commuteSystem.invalidate()`; `CommuteSystem.h:39` |
| 11 | Wrap-around leg not validated | **FIXED** | `Transit.cpp:215-235` loops all `n` pairs with `(i+1)%n` |
| 4 | Population growth O(homes×citizens) | **OPEN** | `Population.cpp:24-33` calls `countResidentsAt` per home; `Population.cpp:97-108` is a full citizen scan |
| 5 | Update exceeds 60 FPS budget | **OPEN** | `Simulation.cpp:36-75` per-frame work; `Employment.cpp:103-163` rebuilds maps every frame; `LandValue.cpp:33-162` 121-tile scans |
| 9 | Release tests cannot fail | **OPEN** | `test_utilities.cpp:1` includes `<cassert>`; `test_utilities.cpp:205-215` always `return 0`; same shape in `test_simulation_balance.cpp` |
| 10 | Apply phase not transactional | **OPEN** | `SaveSystem.cpp:810-818` mutates live world first; `SaveSystem.cpp:856-860` catch cannot undo |
| 12 | Hourly accumulators not persisted | **OPEN** | `Demand.h:59`, `Housing.h:60`, `Happiness.h:63`, `LandValue.h:65` have private `secondsTowardNextHour`; no getter/setter; not in `SaveSystem.cpp` |
| 13 | Route caps unenforced at creation | **OPEN** | `SaveSystem.cpp:39-40` caps; `Transit.cpp:240-250` `createRoute`/`validateRoute` enforce neither |
| 14 | `addMoney` drops negatives | **OPEN** | `Economy.cpp:31-37` `if (amount > 0)`; no non-test call sites |
| 15 | `clockScale` unvalidated on load | **OPEN** | `SaveSystem.cpp:427-434` accepts any float; `SimulationClock.cpp:9-14` `isSupportedSpeed` is file-local |
| 16 | No CI | **OPEN** | no `.github/`; `CMakeLists.txt` has no `enable_testing()`/`add_test()` |
| 17 | README/docs stale | **OPEN** | `README.md:72` says 1920×1080 but `Game.cpp:40` is 1280×720; `README.md:104,113-114,122-124,184`; `ROADMAP.md:19` says v0.1.0 |
| 18 | Utility capacity ignored while paused | **OPEN** | `Utilities.cpp:71-80` early-returns on delta ≤ 0; setters (`Utilities.cpp:189-199`) do not refresh; queries are `const` (`Utilities.h:88-95`) |

**Consequence for the plan:** 7 of the 18 issues (#1, #2, #3, #6, #7, #8, #11) need
**verification + real regression tests + closure**, not new fixes. The other 11 need
code changes. #3 has a small non-wiping residual (money saturates at `INT_MAX`).

---

## 2. Design decisions (pick before Phase 3)

These are the only places where two defensible answers exist. Recommended defaults are
marked **(rec)**. Confirm or override before starting the phase that needs them.

- **D1 — Issue #10 transactional apply.** Option A: make load genuinely
  transactional. **(rec)** Option B: amend the header contract to match reality.
  Recommendation is A, implemented cheaply (see Phase 4).
- **D2 — Money width (#3 residual, #14).** Keep `int` money with clamping **(rec)**,
  or migrate to `int64_t` (touches save format, `getMoney()`, UI `%d`). Recommend
  keeping `int` now; record an ADR for `int64_t`.
- **D3 — Resident counting (#4).** Rebuild a flat `std::vector<int>` count once per
  `Population::update` **(rec)**, or maintain incremental per-home counters.
  Recommend the flat vector: less state to keep consistent and it doubles as the #5
  flat-grid pattern.
- **D4 — Issue #5 scope.** Do the targeted wins first (#4, job-sync gating) and
  **only** flatten the four `std::map` grids if profiling still misses budget **(rec)**.
  The map→vector change touches UI/renderer consumers and deserves its own phase.
- **D5 — Issue #12 save format.** Bump `SAVE_VERSION` to 2 and accept v1 on read
  **(rec)**, rather than an optional trailing record. Explicit and unambiguous.
- **D6 — Issue #18.** Dirty-flag + `mutable` cached `World*` **(rec)**, or
  document-only and defer to the capacity-upgrade feature. Recommend the dirty flag
  so the issue is actually closed.
- **D7 — CI runner.** Windows `windows-latest` + MSYS2 UCRT64 **(rec)**, per the
  issue, because tests link `raylib` and raylib is available there.

---

## 3. Execution phases

Work on a branch: `git switch -c fix/all-open-issues`.

Each phase ends with: clean Debug **and** Release build warning-free
(`-Wall -Wextra -Wpedantic`), `ctest` green in both, in-engine SelfTest (F11) green,
and one commit. A phase that touches shared simulation code must not begin on a dirty
tree.

### Phase 0 — Baseline and safety net

1. Confirm clean tree apart from the pre-existing untracked `.commandcode/`.
2. Build Debug; run both test binaries; run the game once and press F11.
3. Record baseline timings using the harness added in Phase 5 (or the existing
   `build-release` binaries if present) so #4/#5 improvement is measurable.

No issue closes in this phase.

### Phase 1 — Test infrastructure (#9) and CI skeleton (#16, part 1)

**#9** — make test binaries able to fail.

- Add `tests/test_check.h` with a `CHECK(cond)` macro that increments a global
  failure counter, prints `FAIL <file>:<line>: <expr>` to `stderr`, and works under
  `-DNDEBUG`. Add `CHECK_EQ(a,b)` / `CHECK_NEAR(a,b,eps)` helpers as needed.
- Rewrite `tests/test_utilities.cpp` and `tests/test_simulation_balance.cpp`:
  replace every `assert(...)` with `CHECK(...)`; make each `test_*` return `void` as
  today; make `main()` print a summary and `return g_failures == 0 ? 0 : 1;`.
- Remove `<cassert>`.

**#16 (part 1)** — wire CTest and a Debug CI leg.

- `CMakeLists.txt`: add `enable_testing()` and
  `add_test(NAME utilities COMMAND test_utilities)` /
  `add_test(NAME simulation_balance COMMAND test_simulation_balance)`.
- Add `.github/workflows/ci.yml` running on `push` to `main` and `pull_request`:
  `windows-latest`, `msys2/setup-msys2@v2` with `ucrt64` and
  `mingw-w64-ucrt-x86_64-{gcc,cmake,ninja,raylib}`, then configure Debug, build,
  and `ctest --test-dir build --output-on-failure`.

**Acceptance:** intentionally break one `CHECK`, see non-zero exit and a `FAIL` line
in both Debug and Release; `ctest` reports the failure; CI fails on that commit.

Closes **#9**. Advances **#16**.

### Phase 2 — Regression tests for already-fixed issues, then close #1/#2/#3/#6/#7/#8/#11

Add real tests (using the Phase 1 harness) so none of these can silently regress.
These tests are the missing evidence the fix commits described but never committed
(their harnesses lived in gitignored `scratch/`).

- **#1** `tests/test_transit.cpp`: 3-stop loop `[A,B,C,A]` on a connected road row;
  step `Transit::update` until the bus returns to `A`; assert `bus.active`, `path.size() >= 2`,
  and that it completes ≥ 2 full circuits.
- **#2** `tests/test_save_load.cpp`: churn citizens/`nextId` above `MAX_CITIZENS+1`
  with a small live count; `save` then `load`; assert `ok` and live count round-trips.
- **#3** `tests/test_economy.cpp`: seed money near `INT_MAX` with positive net income;
  advance one day; assert money is large and positive, never 0; also assert
  `restoreSavedState` clamps.
- **#6** `tests/test_traffic.cpp`: 3 commuters on a row; soak ≥ 2400 frames; assert
  `nextVehicleId` reaches a flat ceiling and `direction` reverses at both ends.
- **#7** `tests/test_commute.cpp`: routable city; demolish one road and build an
  unrelated road (node count identical); assert no `commutePath` contains a non-road
  tile via both `update()` and after `onWorldModified`.
- **#8** `tests/test_save_load.cpp`: prime commute stats, load a city with identical
  counts but different geometry; assert `sampleRoute` contains no tiles from city A.
- **#11** `tests/test_transit.cpp`: assert a 2-stop and a 3-stop loop route both
  validate and run. (The issue itself records that a failing case is not constructible
  through road transitivity; the fix is defensive. Verify and close as hardening.)

Register each new test file as a CMake target + `add_test`.

**Acceptance:** every test above fails against the pre-fix revision (check with
`git stash`/`git worktree`) and passes at HEAD.

Closes **#1, #2, #3, #6, #7, #8, #11**.

### Phase 3 — Small correctness fixes

Each item: change, test, acceptance.

**#15 — validate `clockScale` on load.**
- `SimulationClock.h`: promote the validator to a public
  `static bool isSupportedSpeed(float scale);` (keep the existing constants).
- `SimulationClock.cpp`: move the anonymous-namespace `isSupportedSpeed` (lines 9-14)
  to the class; `setTimeScale` and `restoreSavedState` use it.
- `SaveSystem.cpp:427-434`: reject when
  `!SimulationClock::isSupportedSpeed(data.clockScale)` (include already present at
  `SaveSystem.cpp:26`).
- Test: `CLOCK <t> 7.5 0` → load fails; `CLOCK <t> 4.0 0` → load succeeds.

**#14 — `addMoney` semantics.**
- `Economy.cpp:31-37`: implement 64-bit add, honour the sign, clamp to
  `[0, INT_MAX]` (mirrors `settleDay`), and update the header comment. (D2.)
- Test: `addMoney(-N)` reduces money and clamps at 0; `addMoney(+N)` clamps at `INT_MAX`.

**#13 — route caps at creation.**
- `Transit.h`: add `static constexpr size_t MAX_ROUTES = 2000;` and
  `static constexpr size_t MAX_STOPS_PER_ROUTE = 5000;`.
- `Transit.cpp` `validateRoute`: reject `stopIds.size() < MIN_ROUTE_STOPS ||
  stopIds.size() > MAX_STOPS_PER_ROUTE`.
- `Transit.cpp` `createRoute`: reject when `routes.size() >= MAX_ROUTES`.
- `SaveSystem.cpp`: delete local `MAX_ROUTES`/`MAX_STOPS_PER_ROUTE` (lines 39-40) and
  use `Transit::MAX_ROUTES` / `Transit::MAX_STOPS_PER_ROUTE` (mind signed/unsigned
  compares).
- Test: exceed the route cap → `createRoute` returns false; a route longer than the
  stop cap → `validateRoute` false; near-cap save/load round-trips.

**#12 — persist hourly accumulators.**
- Add to `LandValue`, `Housing`, `Happiness`, `Demand`:
  `float getSecondsTowardNextHour() const` and
  `void restoreSavedState(float secondsToward_)` clamping to `[0, SIM_SECONDS_PER_HOUR)`.
- Save (`SaveSystem.cpp` save, after `CAMERA` or near `POLLUTION`): write a
  `HOURCLOCKS <landValue> <housing> <happiness> <demand>` record; bump
  `SaveSystem::SAVE_VERSION` to 2 (D5).
- Load: accept version 1 (accumulators default 0) and version 2 (parse `HOURCLOCKS`).
  Range-check each against `[0, 3600)`.
- Apply: call each `restoreSavedState`.
- Test: save at a known hour-phase, load, assert all four values round-trip; and a
  hand-written v1 file still loads.

**#18 — capacity changes apply while paused.**
- `Utilities.h`: add `mutable bool gridDirty = true;` and a private non-const
  `void refreshIfDirty() const;` (uses a `mutable const World* cachedWorld`);
  set `cachedWorld` + clear `gridDirty` in `update()`/`recalculate()`.
- `Utilities.cpp` setters (189-199): set `gridDirty = true` after clamping.
- Const queries (`getTileStatus`, `isTileSupplied`, `getElectricityDemand`, capacity
  and count getters, `getStatusGrid`) call `refreshIfDirty()`; if `cachedWorld` is
  null or the grid was never built, do nothing.
- Test: 60 commercial tiles, 100 capacity → some unsupplied; raise capacity;
  call `update(0.0f)`; assert supply now reflects the new capacity.

Closes **#14, #15, #13, #12, #18**.

### Phase 4 — Transactional load (#10) — decision D1

Recommended (Option A), implemented with a snapshot taken **before** the first
mutation and restored via `noexcept` moves in the catch:

1. `World`: add `std::vector<Tile> snapshotTiles() const` and
   `void restoreTiles(std::vector<Tile> tiles) noexcept`.
2. `SaveSystem::load`, immediately before line 810 (first mutation):
   - `Simulation simulationSnapshot = simulation;`
   - `std::vector<Tile> tileSnapshot = world.snapshotTiles();`
   - `Camera cameraSnapshot = camera;`
   (These copies happen before anything is mutated; if they throw, the live city is
   untouched.)
3. In `catch (...)` around apply: `simulation = std::move(simulationSnapshot);`
   `world.restoreTiles(std::move(tileSnapshot));` `camera = cameraSnapshot;` then
   return `"apply failed: ..."`. Move-assignments are `noexcept`, so the restore
   itself cannot throw.
4. Keep the header contract at `SaveSystem.h:31-33`.

If D1 is Option B instead: change `SaveSystem.h:31-33` to state that parsing is
transactional and that apply is ordered so no fallible step precedes the first
mutation, and drop the snapshot.

- Test seam (test builds only): compile-time `URBANIA_LOAD_FAILPOINT` that throws once
  right after the tile commit. Test asserts the world, citizens, economy, clock, and
  camera are byte-for-byte unchanged and `load` returns `ok == false`.

Closes **#10**.

### Phase 5 — Fix #4 (population quadratic)

- `Population.h`: add `std::vector<int> residentCounts;` (flat, indexed
  `y * width + x`).
- `Population::update` (`Population.cpp:12-46`): after `syncWithWorld`, one pass over
  `citizens.getCitizens()` fills `residentCounts` (zero-fill then increment per valid
  home). The growth loop (lines 18-37) replaces both `countResidentsAt` calls with
  `residentCounts[y*width+x]`, incrementing on each `createCitizen`.
- `getResidentsAt` (`Population.cpp:58-65`) returns the cached flat count.
- Keep `countResidentsAt` only if still referenced; otherwise remove it.
- Sizing: lazily `residentCounts.assign(width*height, 0)` from `world.getWidth()` /
  `getHeight()` inside `update`.
- Existing tests call `population.update(...)` before `getResidentsAt(...)`
  (`test_simulation_balance.cpp:39,48,69,75,95`, `SelfTest.cpp:105-106,350-352`), so
  the cache is fresh for them — no test change needed.
- Add `tests/bench_population.cpp` (not CI-gating): 300 and 3000 homes, print per-update
  ms; assert linear scaling, and the invariant
  `sum(getResidentsAt over homes) == getTotalPopulation()`.

**Acceptance:** the 3000-home update drops from the ~100 ms range recorded in the
issue to a small multiple of the 300-home case; invariant holds.

Advances **#5**. Closes **#4**.

### Phase 6 — Broader frame budget (#5), measure-first

Order (per D4):

1. **Job-sync gating.** `Employment` currently rebuilds two `std::map`s every frame
   (`Employment.cpp:103-163`). Add a dirty flag set by `Simulation::onWorldModified`,
   `initialize`, and `rebuildAfterLoad`; `Employment::update` skips
   `syncJobsWithWorld` when clean. Keep `reconcileOccupancy`/`matchUnemployed` as-is.
2. **Re-measure** with the Phase 5 harness on a dense 80×80 city at 8x. If the
   representative update is ≤ 16.67 ms, stop here and document the numbers.
3. **Only if still over budget:** flatten the four `std::map<TileCoordinate, …>` grids
   (`Pollution.h:57`, `LandValue` `grid`, `Housing.h:55`, `Utilities.h:131`) to
   `std::vector`. This changes the grid-return APIs used by renderers/UI
   (`getPollutionGrid`, `getLandValueGrid`, `getResidentialGrid`, `getStatusGrid`),
   so update every consumer and keep the save format unchanged (still POL/LANDVALUE
   keyed records).
4. **Only if still over budget:** replace `LandValue`'s per-tile 121-tile park scan
   (`LandValue.cpp:116-135`) with a two-pass dilation.

**Acceptance:** dense city update ≤ 16.67 ms at 8x on the reference machine, or the
remaining gap is documented with measurements and a follow-up issue.

Closes **#5**.

### Phase 7 — Complete CI (#16, part 2)

- Add a Release matrix leg to `.github/workflows/ci.yml` **now that #9 is fixed**.
- Add the negative test: a `tests/test_harness_negative.cpp` with a deliberate
  `CHECK(false)`, built only under a CMake option (e.g. `URBANIA_NEGATIVE_TESTS`,
  default OFF); a CI step enables it and asserts the binary exits non-zero. This
  proves the harness can fail.
- Optional: add `CMakePresets.json` (`dev`, `release`) and update README build steps.

Closes **#16**.

### Phase 8 — Documentation (#17)

Single pass over the three docs:

- `README.md`: window default 1280×720 (`README.md:72`); controls table add `TAB`
  (dashboard, `Game.cpp:205-208`) and mention bus-stop cost (Rs. 500,
  `Transit.h:40`); remove "There is no income yet" (`:104`); update Citizens "no
  movement or rendering yet" (`:113-114`); replace "temporary debug info" HUD
  description (`:122-124`); fix "Not yet implemented" list that still includes
  textures/save-load (`:184`); note `R`/`B` route/stop controls where relevant.
- `docs/ROADMAP.md`: change "Current State: Baseline v0.1.0" (`:19`) to v1.0.0 and
  reconcile the shipped-features list.
- `docs/PROGRESS.md`: append the phases from this plan as they land.

No automated test; acceptance is a reviewer diff against the actual behavior in
`Game.cpp` and `Transit.h`.

Closes **#17**.

### Phase 9 — Final audit and closure

1. Full clean Debug + Release builds, warning-free.
2. `ctest` green in both configs; negative test proves failure detection.
3. In-engine F11 SelfTest green on a fresh and a loaded city.
4. Manual smoke: loop bus route runs; save/load after >100k IDs; paused capacity
   change; corrupt `CLOCK` rejected; hour-phase round-trip.
5. Confirm CI green on the branch and after merge to `main`.
6. Close every issue with a comment naming the commit + test, including the
   already-fixed seven (`#1,#2,#3,#6,#7,#8,#11`) with their new regression tests.

---

## 4. Regression test matrix

| Issue | New/changed test | Asserts |
|-------|------------------|---------|
| 1 | `test_transit.cpp` | loop bus stays active, ≥2 circuits |
| 2 | `test_save_load.cpp` | `nextId > MAX_CITIZENS` round-trips |
| 3 | `test_economy.cpp` | near-`INT_MAX` day settlement stays positive |
| 4 | `bench_population.cpp` | linear scaling; count-sum invariant |
| 5 | `bench_simulation.cpp` | update ≤ 16.67 ms on dense city |
| 6 | `test_traffic.cpp` | vehicle IDs bounded over soak |
| 7 | `test_commute.cpp` | no stale path after net-zero road edit |
| 8 | `test_save_load.cpp` | loaded city shows its own `sampleRoute` |
| 9 | `test_harness_negative.cpp` | deliberate failure exits non-zero |
| 10 | `test_save_load.cpp` (+ load failpoint) | failed apply leaves city unchanged |
| 11 | `test_transit.cpp` | 2/3-stop loop routes validate and run |
| 12 | `test_save_load.cpp` | four hour-accumulators round-trip; v1 load works |
| 13 | `test_transit.cpp` | caps enforced at creation; near-cap round-trip |
| 14 | `test_economy.cpp` | `addMoney` sign + clamp semantics |
| 15 | `test_save_load.cpp` | bad `clockScale` rejected, city untouched |
| 16 | CI + negative test | build/test on push; harness provably fails |
| 17 | (manual) | README/ROADMAP match behavior |
| 18 | `test_utilities.cpp` | capacity change applies while paused |

---

## 5. Build & verify commands

MSYS2 UCRT64, from the repo root (raylib installed):

```sh
cmake -S . -B build -G Ninja -DCMAKE_BUILD_TYPE=Debug
cmake --build build
ctest --test-dir build --output-on-failure

cmake -S . -B build-release -G Ninja -DCMAKE_BUILD_TYPE=Release
cmake --build build-release
ctest --test-dir build-release --output-on-failure
```

Both builds must be free of `-Wall -Wextra -Wpedantic` warnings.

---

## 6. Risks and rollback

- **#10 snapshot cost.** One transient deep copy of `Simulation` on each load; load is
  rare and user-initiated. If memory is a concern, switch to the move-out variant.
- **#12 format change.** Bumping `SAVE_VERSION` means old binaries reject new saves.
  Mitigate by accepting v1 on read; document in the header layout block.
- **#5 grid refactor.** Largest blast radius (renderers/UI read the grids). Gated
  behind measurement; if attempted, land it as its own commit with a visual smoke test
  of F7–F10 overlays and the tile inspector.
- **#18 cached `World*`.** Valid only while `Utilities` and `World` share lifetime
  (they do: `Game` owns both). If `Utilities` ever outlives `World`, this must change.
- Every phase is a separate commit on `fix/all-open-issues`; any phase can be reverted
  independently without affecting the others.

---

## 7. Definition of done

- All 18 issues closed, each with a commit link and the test that guards it.
- Debug and Release build warning-free; `ctest` green in both.
- Negative test demonstrates the harness can fail.
- CI runs build + tests on push and PR; Release leg present.
- README, ROADMAP, PROGRESS reflect v1.0.0 behavior.
- Dense-city simulation update within the 60 FPS budget at 8x, or a documented,
  measured follow-up.

---

## 8. Issue closure checklist

- [ ] #1 loop bus route — regression test, close
- [ ] #2 citizen ID cap — regression test, close
- [ ] #3 treasury overflow — regression test, close (note `INT_MAX` saturation, D2)
- [ ] #4 population quadratic — fix + benchmark, close
- [ ] #5 frame budget — measure, fix as needed, close
- [ ] #6 vehicle churn — regression test, close
- [ ] #7 stale commute paths — regression test, close
- [ ] #8 `CommuteSystem` reset — regression test, close
- [ ] #9 Release tests — CHECK harness + ctest, close
- [ ] #10 transactional load — D1, fix + failpoint test, close
- [ ] #11 wrap leg — regression test, close as hardening
- [ ] #12 hour accumulators — record + version bump + test, close
- [ ] #13 route caps — Transit constants + test, close
- [ ] #14 `addMoney` — semantics + test, close
- [ ] #15 `clockScale` — public validator + test, close
- [ ] #16 CI — workflow + CTest + Release + negative test, close
- [ ] #17 docs — README + ROADMAP + PROGRESS pass, close
- [ ] #18 paused capacity — dirty flag + test, close
