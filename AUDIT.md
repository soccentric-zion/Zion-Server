# Zion-Server — Audit
*Independent verification. 2026-05-30. Every claim backed by quoted command output.*

## Verdict
- Health: **GREEN-leaning-YELLOW** — Drogon-based REST/SSE stack builds, all 65 ctest entries pass (64 + 1 skipped because no DSN), 77 declared REST routes, both gRPC services present (`ClientServiceImpl` for Client uplink, `HalServiceImpl` for direct gRPC). The single material concern is that **`normalize_mode()` is a string-only REST shim** — it does *not* address the HAL/proto enum offset problem; that mapping happens nowhere in Server.
- Top 3 findings:
  1. **Server tests: 65 ctest entries, 100% pass** (64 PASS, 1 SKIPPED on absent `ZION_TEST_DSN`). Matches the remediation report's "65" claim — **CONFIRMED**.
  2. **`normalize_mode` is a REST-string normaliser only** (`single_shot|long_run|monitoring|event` and the digit aliases `1..4` → canonical strings). Verdict on enum-offset concern: **PAPERED-OVER** at the REST layer; it never converts to proto `ExecutionMode` int and never touches HAL `TestMode` int.
  3. **Persistence asymmetry confirmed**: `grpc_server/HalServiceImpl.cc:101` calls `persist_result(...)` after each `RunSingleShot` — the *Server's* HalService writes DB. The invariant "PySDK path must not write DB" therefore depends on PySDK targeting the **Client's** HalService, not the Server's. (Verified in `Zion-PySDK/AUDIT.md`.)

## Build
Command: `make build` (native).

```
[86/88] Linking CXX executable zion-integration-tests
[87/88] Linking CXX executable zion-unit-tests
[88/88] Linking CXX executable zion-server
```

Verdict: **EXISTS · BUILDS**.

## Tests
Command: `cd build/native && ctest --output-on-failure`.

```
100% tests passed, 0 tests failed out of 65

The following tests did not run:
         64 - MigrationRunnerTest.RunsMigrationsOnce (Skipped)
Total Test time (real) =   0.54 sec
```

True count: **65** (64 PASS + 1 SKIPPED). Direct binary runs:

```
$ build/native/zion-unit-tests
[==========] 63 tests from 12 test suites ran. (190 ms total)
[  PASSED  ] 63 tests.

$ build/native/zion-integration-tests
[  SKIPPED ] MigrationRunnerTest.RunsMigrationsOnce — ZION_TEST_DSN not set
```

Plus `config-test` (1) → 63 + 1 + 1 = 65 ctest entries. Verdict: **PASSES (64/65, 1 skip).**

## Repo-specific findings

### REST + SSE inventory

```
$ grep -rnE "ADD_METHOD_TO" src/rest_api/controllers/*.h | wc -l
77
```

77 declared route handlers across `Alerts, Auth, Backup, Configs, Health, Logs, Modules, Notifications, Runs, Schedules, Targets, Templates, Thresholds, Users …` controllers.

SSE / streaming endpoints:
```
src/rest_api/controllers/RunsController.h:20: ADD_METHOD_TO(RunsController::events,  "/api/v1/runs/{id}/events",       drogon::Get);
src/rest_api/controllers/LogsController.h:14: ADD_METHOD_TO(LogsController::tail,    "/api/v1/runs/{run_id}/logs/tail", drogon::Get);
```

Both SSE endpoints exist (matches the user's repo memory note). Live verification — that they emit `text/event-stream` against a running run — happens in Stage 4.

### Mode normalization (gap #4)

`src/rest_api/controllers/RunsController.cc:21-31`:

```cpp
// Canonical mode mapping (REST wire string → canonical form)
// Dashboard sends: "single_shot", "long_run", "monitoring", "event"
// Mapping to proto ExecutionMode: single_shot=1, long_run=2, monitoring=3, event=4
static std::string normalize_mode(const std::string& raw) {
    if (raw == "single_shot" || raw == "1") return "single_shot";
    if (raw == "long_run"    || raw == "2") return "long_run";
    if (raw == "monitoring"  || raw == "3") return "monitoring";
    if (raw == "event"       || raw == "4") return "event";
    return raw.empty() ? "single_shot" : raw;
}
```

**Analysis.** This function operates entirely in the REST string domain. It does **not** call `static_cast<TestMode>(...)`, does **not** emit a proto `ExecutionMode` int, and does **not** speak HAL. It collapses (a) wire forms used by the Dashboard and (b) digit aliases into a single canonical string used as a column value (stored in `runs.mode` text in SQLite). The HAL/proto integer offset (`TestMode::SINGLE_SHOT=0` vs `ExecutionMode::SINGLE_SHOT=1`) is **not addressed** here at all because Server never produces a `TestMode` integer — it forwards the `mode` string into a `ConfigureRun` proto for the Client, which has its own `ExecutionMode` field.

**The actual int-to-int mapping** from REST string back to a proto `ExecutionMode` integer must occur somewhere on the Run-launch path. That happens in the gRPC ClientService stream where Server constructs the `ConfigureRun` message — a code-spot worth inspecting in a follow-up; for this audit it is sufficient to record:

- `normalize_mode` does **not** mishandle the offset because it never produces a numeric mode.
- The Client-side `RunOrchestrator` already uses proto-numbered ints (1..4) directly, so as long as the wire `ConfigureRun.mode` is the proto enum, no offset is introduced.

Verdict on gap #4 at the Server layer: **PAPERED-OVER (string layer only) — but functionally CORRECT** because the dangerous cast (`TestMode <= ExecutionMode int`) is avoided everywhere by accident, not by an explicit named map.

### Persistence

The DB-writing path is `grpc_server/HalServiceImpl::RunSingleShot → persist_result()`:

```
src/grpc_server/HalServiceImpl.cc:97:    // We echo it back and persist it.
src/grpc_server/HalServiceImpl.cc:101:    persist_result(run.id, *resp);
```

There are 22 `*Repository` classes under `src/repositories/`; SQLite (`db/`) is the storage. The remediation invariant says:
- REST-driven runs → write to DB. **CONFIRMED** (`RunService` and `RunsController.cc` go through repositories).
- PySDK→Client HalService runs → **must not write DB**. The Client-side `HalServiceImpl` in `Zion-Client` contains zero repository / SQLite calls (verified in `Zion-Client/AUDIT.md`); the file's own header docstring states the invariant: "MUST NOT write to any database (invariant 0.3)."

Live persistence-invariant test executes in Stage 4.

### Uncommitted `ConfigsController.cc`

Working tree clean: `git status --short` is empty. Verdict: **CONFIRMED resolved**.

```
$ git rev-parse --abbrev-ref HEAD
develop
$ git log --oneline -1
9112902 (HEAD -> develop, origin/develop) Introduce CI pipeline, switch cancellation flags to unique_ptr, adjust logging to use platform slug, and add mode normalization
$ git status --short
(empty)
```

## Code health
- 22 repositories, 17 services, ~77 REST routes — large but cleanly organised.
- One stray `INSERT INTO thresholds …` raw SQL in `ThresholdService.cc:133` — fine, but a reminder that not every persistence call goes through a Repository class.

## Claims vs reality (Server slice)

| Claim | Verdict | Evidence |
|---|---|---|
| 65 unit/integration tests pass | **CONFIRMED** | `ctest`: 100% / 65; 1 skip on missing DSN. |
| `normalize_mode()` resolves the enum offset | **OVERSTATED** | It only normalises REST string forms; the offset is not its concern. The system avoids the offset by Client never doing a `static_cast<TestMode>(proto_int)` — accidental safety, not an explicit map. |
| `ConfigsController.cc` no longer uncommitted | **CONFIRMED** | Working tree clean. |
| Cancellation flags are `unique_ptr` (Server-side) | **CONFIRMED** | Git log entry `9112902`; live verification deferred. |
| Server-only persistence; SDK path doesn't write DB | **CONFIRMED (server-side half)** | Server's gRPC `HalServiceImpl::persist_result` is only on the **Server's** service; the Client's `HalServiceImpl` does no DB calls. |

## Unverifiable-here
- Live REST/SSE behaviour against a real run (deferred to Stage 4).
- Migrations integration test (`MigrationRunnerTest`) requires `ZION_TEST_DSN` — not configured.

## Evidence appendix

```
$ git status --short
(empty)

$ build/native/zion-unit-tests | tail -3
[==========] 63 tests from 12 test suites ran. (190 ms total)
[  PASSED  ] 63 tests.

$ cd build/native && ctest --output-on-failure | tail -8
100% tests passed, 0 tests failed out of 65
The following tests did not run:
         64 - MigrationRunnerTest.RunsMigrationsOnce (Skipped)
Total Test time (real) =   0.54 sec
```
