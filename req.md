First Review the Acaida-HAL and Zion-Server repo 



# Server App Update — Align with HAL Refactor

You are updating the **Server App** (C++ on x86, gRPC for the on-target Client, REST + OpenAPI for the Web UI, PostgreSQL persistence) to consume the **refactored HAL result types**. The HAL has been refactored to a new canonical `TestResult` / `TestOutput` shape, and the shared protos have been updated in lockstep. The Server App must now be updated everywhere it touches results: gRPC handlers, persistence schema, REST surface, OpenAPI, analytics (trend / compare / baseline / threshold), reports, notifications, and tests.

This is a long-running task. Time is not a constraint. **Correctness, completeness, and consistency are the only constraints.**

---

## 1. Background — what changed upstream

The HAL's canonical types are now:

```cpp
using Value = std::variant<bool, int64_t, double, std::string>;

enum class Status   { PASS, FAIL, ERROR, TIMEOUT, SKIPPED, ABORTED };
enum class Severity { INFO, WARNING, ERROR, CRITICAL };

struct TestOutput {
    Status status;
    Severity severity;
    std::map<std::string, std::string> reason;
    std::map<std::string, Value> metrics;
    std::map<std::string, Value> monitoring;
    std::map<std::string, Value> event;
    std::string error_message;
};

struct TestResult {
    std::string module_id;
    std::string test_id;
    std::string name;
    uint64_t sequence;
    std::chrono::system_clock::time_point timestamp;
    std::chrono::nanoseconds duration;
    TestOutput output;
    std::vector<TestResult> subtests;
};
```

The mirroring `hal.common` proto now defines `TestStatus`, `TestSeverity`, `TypedValue` (oneof bool/int64/double/string), `TestOutput`, and `TestResult`. Read `protos/common.proto` for exact tag numbers, enum ordering, and field names. **Do not assume — read.**

The Client App has already been updated to produce the new shape over the wire. The Server App must now ingest, persist, query, analyze, render, and report on it correctly.

Old fields that must disappear: `bool passed`, `std::vector<std::string>` or `std::map<std::string,std::string>` named `data`, `duration<double>`. New fields that must work everywhere: `status` (6-value enum), `severity` (4-value enum), `reason` / `metrics` / `monitoring` / `event` typed maps, `error_message`, `module_id` / `test_id` / `sequence` / `timestamp` / `duration` on the result, and recursive `subtests`.

---

## 2. Scope of this update

You will touch:

1. **Proto-binding regen** — Server links against generated C++ from the updated `protos/`. Regenerate, rebuild.
2. **gRPC handlers** — every server-streaming and unary handler that receives `ResultEnvelope` / `TestResult`. Status, severity, typed maps, subtests, error_message — all must be honored.
3. **Persistence schema** — PostgreSQL schema migration. The `results` table grows fields for the new shape. Storage of `metrics` / `monitoring` / `event` happens as JSONB (or normalized companion tables, see Section 4.3). Old `passed` column retired with a documented retention policy.
4. **Repositories** — DAO layer that reads/writes results. Update queries, indexes, projections.
5. **Services** — `ResultService`, `RunService`, `ModuleService`, etc. Business logic now operates on `status` (not boolean), `severity`, typed maps. PASS/FAIL/NEVER summaries must understand the wider status set.
6. **REST controllers** — every endpoint returning result data. JSON shape changes: `status` string, `severity` string, typed values that preserve type in JSON, subtests serialized recursively.
7. **OpenAPI** — schema definitions updated; the Web UI regenerates its TypeScript client from this.
8. **Analytics** — trend, compare, baseline diff, threshold evaluation, histogram, anomaly. All must operate on `metrics` (typed numerics now), not on stringified values.
9. **Reports** — PDF / HTML generation includes the new fields; templates updated.
10. **Notifications** — failure detection now triggers on `status != PASS` (and respects severity); update the rules.
11. **Audit log entries** — any audit entry that references result fields.
12. **Tests** — unit, integration, end-to-end. New shape everywhere. Status/severity matrix coverage.

You will **not** touch:
- The HAL (already refactored).
- The protos (already updated).
- The on-target Client App (already updated).
- The Web UI (separate prompt).

---

## 3. Architectural rules — reaffirmed

These were load-bearing in the original Server design. The refactor must preserve all of them:

- **Single results table.** All results — every module, every status — share one table. The new typed maps (`metrics`, `monitoring`, `event`) are stored as JSONB columns on that one table. **Do not** create per-module result tables.
- **Module registry is the source of truth.** Server reads `protos/registry/modules.yaml`. The set of valid `module_id` values comes from there. The refactor does not introduce hardcoded module knowledge.
- **REST mirrors gRPC.** Every concept on the gRPC surface has a REST counterpart; the OpenAPI document is the authoritative REST contract for the Web UI.
- **No `switch(module_id)`** outside an explicit registry. The refactor must not introduce module-aware special cases in handlers, services, or controllers.
- **Streaming backpressure preserved.** Bounded queues; if DB writer falls behind, propagate backpressure to the client by stalling reads. Never drop, never OOM. The new larger payloads make this more important, not less.
- **Async DB sessions only.** Do not introduce sync DB paths to "simplify" the migration.
- **gRPC and REST do not share thread pools.** Existing threading model preserved.

---

## 4. Required code changes — concrete

### 4.1 Proto bindings

1. Regenerate Server's C++ proto bindings from the updated `protos/`.
2. Confirm the build configures and links cleanly. Old generated symbols (`set_passed`, `mutable_data`, etc.) must be gone.
3. If any non-generated code still calls those symbols, that's a Section 4.x change.

### 4.2 gRPC handlers

For every handler under `src/grpc_server/handlers/` that processes `ResultEnvelope` / `TestResult`:

- Replace `result.passed()` reads with `result.output().status()`.
- Replace `result.data()` reads with the four typed maps from `output`.
- Read `output.severity()`, `output.error_message()`.
- Read top-level `result.module_id()`, `result.test_id()`, `result.sequence()`, `result.timestamp()`, `result.duration()`.
- Recursively process `result.subtests()`.
- Reject envelopes with `TEST_STATUS_UNSPECIFIED` — return `INVALID_ARGUMENT` with a clear message. The Client should never send unspecified, but defend.
- Validate `module_id` against `ModuleRegistry`. If unknown, reject — same as before, but now `module_id` comes from the result itself, not the envelope.

Each streaming handler persists each `TestResult` as it arrives, via the new repository signature (Section 4.4). Backpressure unchanged.

### 4.3 Persistence schema

Write a new Alembic-style SQL migration (additive only). The `results` table is the primary surface.

**New columns on `results`:**

```sql
ALTER TABLE results
    ADD COLUMN status        SMALLINT      NOT NULL DEFAULT 0,
    ADD COLUMN severity      SMALLINT      NOT NULL DEFAULT 0,
    ADD COLUMN test_id       TEXT          NOT NULL DEFAULT '',
    ADD COLUMN test_name     TEXT          NOT NULL DEFAULT '',
    ADD COLUMN sequence      BIGINT        NOT NULL DEFAULT 0,
    ADD COLUMN duration_ns   BIGINT        NOT NULL DEFAULT 0,
    ADD COLUMN error_message TEXT          NOT NULL DEFAULT '',
    ADD COLUMN reason        JSONB         NOT NULL DEFAULT '{}'::jsonb,
    ADD COLUMN metrics       JSONB         NOT NULL DEFAULT '{}'::jsonb,
    ADD COLUMN monitoring    JSONB         NOT NULL DEFAULT '{}'::jsonb,
    ADD COLUMN event         JSONB         NOT NULL DEFAULT '{}'::jsonb,
    ADD COLUMN parent_id     UUID          NULL REFERENCES results(id) ON DELETE CASCADE;
```

Notes on each:

- `status` and `severity` stored as `SMALLINT` matching the proto enum integer values. **Never** repurpose enum integer values. Add a comment in the migration listing the integer-to-name mapping.
- `module_id`, `module_version`, `run_id`, `timestamp`, `payload_bytes`, `payload_type_url` — keep as-is from the original schema. The new structured columns are projections of the proto payload for efficient querying; `payload_bytes` still holds the full serialized proto for fidelity.
- `parent_id` enables recursive `subtests` storage in the same table. Top-level results have `parent_id = NULL`. Subtests reference their parent. Cascade delete keeps cleanup easy.
- All JSONB columns default to `'{}'` so old rows continue to query.

**`TypedValue` encoding in JSONB:**

Store each typed value with explicit type tagging so the original variant kind is preserved on round-trip. Use this object shape:

```json
{ "type": "double", "value": 42.5 }
{ "type": "int",    "value": 1234 }
{ "type": "bool",   "value": true }
{ "type": "string", "value": "high" }
```

Apply uniformly across `metrics`, `monitoring`, `event`. The top-level JSONB object is a map of `key → typed value`. This preserves typing on the way to the Web UI's chart code without forcing it to guess.

**Indexes:**

```sql
CREATE INDEX results_status_ts_idx       ON results (status, timestamp DESC);
CREATE INDEX results_module_test_ts_idx  ON results (module_id, test_id, timestamp DESC);
CREATE INDEX results_run_sequence_idx    ON results (run_id, module_id, sequence);
CREATE INDEX results_parent_idx          ON results (parent_id);
CREATE INDEX results_metrics_gin_idx     ON results USING GIN (metrics);
CREATE INDEX results_monitoring_gin_idx  ON results USING GIN (monitoring);
```

The GIN indexes are required for any filter like `metrics ? 'throughput_mbps'` or `metrics @> '{"key":{"value":...}}'`. Without them, threshold and trend queries scan the whole table.

**Retiring `passed`:**

- Keep the existing `passed BOOLEAN` column for now. Mark deprecated in a comment.
- Backfill: `UPDATE results SET status = CASE WHEN passed THEN 1 ELSE 2 END WHERE status = 0;` — assuming `PASS = 1, FAIL = 2` per the proto enum. **Verify the actual numeric values from `common.proto` before running** — the proto enum dictates these.
- Add a constraint after backfill: `ALTER TABLE results ADD CONSTRAINT results_status_set CHECK (status > 0);` (i.e., status must not be `UNSPECIFIED`).
- Schedule `passed` for drop in a future migration once nothing reads it. Do not drop in this migration.

**Run-level summary materialization (optional but recommended):**

Add a `run_summaries` table (or materialized view) keyed by `(run_id, module_id, test_id)` storing the last result's status, severity, timestamp, and a few aggregates. The "per-module last result before run ended" view on the run detail page becomes a single indexed lookup instead of a window function over results. Document choice and refresh strategy.

### 4.4 Repository layer

Update `ResultRepository` (and any companion):

- `insert(TestResult)` accepts the proto and writes a full row including recursive subtests. Subtests are inserted with `parent_id = <top_result.id>` in a single transaction.
- `find_by_run(run_id, filters)` — supports filters on `status`, `severity`, `module_id`, `test_id`, time range.
- `find_last_per_module(run_id)` — returns the last result per (module_id, test_id) for the run. Use `run_summaries` if implemented; else a window function.
- `find_module_trend(module_id, test_id, field, from, to)` — returns time-series values of `metrics->>field` (typed-aware extraction). Used by trend view.
- `find_distribution(module_id, test_id, field, runs)` — values for histogram.
- `find_for_compare(run_a, run_b)` — returns per-module final status pairs for compare view.
- All result reads return the typed map structure unchanged (don't flatten in the repository — let services do the projection).

Add new repository methods for `parent_id` traversal: `get_subtests(parent_id)`. Keep it lazy — don't auto-hydrate full subtree on every fetch; let callers ask.

### 4.5 Service layer

#### `ResultService`
- API surface changes from `bool passed` everywhere to `TestStatus status`, plus optional severity.
- New helper: `is_terminal_failure(status, severity)` → bool, used by notification rules. `FAIL` with `ERROR`/`CRITICAL` qualifies; `SKIPPED` does not; `TIMEOUT` does; `ABORTED` does not (user-initiated).
- Aggregation logic for per-module run summary now produces `(status, severity, last_timestamp)` not `(passed, timestamp)`.

#### `RunService`
- Per-module summary: PASS / FAIL / NEVER badge in the original spec must now account for the wider status set. Rule:
  - **PASS** — last result has `status == PASS`.
  - **FAIL** — last result has `status ∈ {FAIL, ERROR, TIMEOUT}`.
  - **NEVER** — module enabled in this run but no result ever produced. **Or** all results were `SKIPPED`.
  - **ABORTED** — surface separately when the last result is `ABORTED`. Display as a distinct badge in the UI later; service exposes it.
- Run terminal status derivation: a run is `failed` if any enabled module's last result is in {FAIL, ERROR, TIMEOUT} and severity is ERROR or CRITICAL; otherwise `completed`. Document edge cases.

#### `ThresholdService` (analytics)
- Thresholds are configured per `(module_id, test_id, metric_key)` with an op and a typed value.
- Evaluation: extract `metrics->key` as a `TypedValue`, compare per op. Supported ops at minimum: `GT, LT, GE, LE, EQ, NE`. Document if regex / range / dwell support is in or deferred.
- A crossing produces a notification (Section 4.10).

#### `AnomalyService`
- Z-score or IQR over recent N results of a `(module_id, test_id, metric_key)`. Only numeric `Value` variants participate.
- Flagged results get an `is_anomaly = true` projection in result reads; not stored in DB unless persistence is needed for UI performance — decide and document.

#### `BaselineService`
- A run can be marked baseline. Comparison of any later result against baseline produces per-metric drift (delta and percent), per-module status changes (regressed / recovered / new failure / new pass), and a summary.
- Baseline metric extraction must respect typed values — numeric drift on numerics, set diff on strings, etc.

#### `CompareService`
- Two runs side-by-side: per `(module_id, test_id)`, compare last-status, last-severity, and per-metric values. Output `unchanged / improved / regressed / drifted / disappeared / appeared`.

#### `ReportService`
- PDF/HTML generation now renders status as a string (`PASS` / `FAIL` / `ERROR` / `TIMEOUT` / `SKIPPED` / `ABORTED`), shows severity, renders typed metrics with units when known (units come from module schema if present, else raw value), recursively renders subtests as nested sections.
- Old templates that displayed "Passed: true/false" must be updated. If the report template is user-customizable, ship a default that uses the new fields and a migration note for anyone with custom templates.

### 4.6 REST controllers and DTOs

For every controller under `src/rest_api/controllers/` that returns result data, update the response DTO to reflect the new shape.

**Result DTO (JSON):**

```json
{
  "id": "uuid",
  "run_id": "uuid",
  "module_id": "cpu",
  "module_version": 1,
  "test_id": "cpu_stress",
  "name": "CPU Stress Test",
  "sequence": 42,
  "timestamp": "2026-05-14T19:40:00.123456Z",
  "duration_ns": 154200000,
  "status": "PASS",
  "severity": "INFO",
  "error_message": "",
  "reason":     { "why": "all assertions held" },
  "metrics":    { "throughput_mbps": { "type": "double", "value": 12345.6 } },
  "monitoring": { "cpu_temp_c":      { "type": "double", "value": 68.2 } },
  "event":      {},
  "subtests":   [ { /* recursive */ } ],
  "payload_type_url": "type.googleapis.com/hal.modules.cpu.Result",
  "payload_b64": "..."
}
```

Rules:
- `status` and `severity` are uppercase strings, not numbers, on the REST surface. Document the enum-name canonicalization in OpenAPI.
- Typed values preserve the `{type, value}` object form. The Web UI's renderer uses this to decide chartability.
- `duration_ns` exposed as int64 nanoseconds; the UI converts for display.
- `timestamp` ISO 8601 UTC with microsecond precision.
- `payload_b64` and `payload_type_url` retained for clients that want the raw proto. The structured fields above are projections.

**Affected endpoints (verify exact paths):**

- `GET /api/v1/runs/{id}/results`
- `GET /api/v1/runs/{id}` (per-module summary section)
- `GET /api/v1/results/{id}`
- `GET /api/v1/results` (cross-run query)
- `GET /api/v1/runs/{a}/compare/{b}`
- `GET /api/v1/modules/{id}/trend`
- `GET /api/v1/modules/{id}/distribution`
- `GET /api/v1/modules/{id}/heatmap`
- `POST /api/v1/runs/{id}/baseline`
- `GET /api/v1/runs/{id}/results/export?format=csv|json`

CSV export now emits the new columns. Column order:
`run_id, module_id, test_id, name, sequence, timestamp, duration_ns, status, severity, error_message, reason_json, metrics_json, monitoring_json, event_json`.

Nested subtests in CSV are flattened with a `parent_result_id` column. Document the format in the README.

JSON export streams the same DTO as above, one result per line (NDJSON) for large runs.

### 4.7 OpenAPI document

Update the generated OpenAPI 3 spec:

- New `TestStatus` enum: `PASS`, `FAIL`, `ERROR`, `TIMEOUT`, `SKIPPED`, `ABORTED`.
- New `TestSeverity` enum: `INFO`, `WARNING`, `ERROR`, `CRITICAL`.
- New `TypedValue` schema: oneOf of `{type:"bool",value:boolean}`, `{type:"int",value:integer}`, `{type:"double",value:number}`, `{type:"string",value:string}`.
- New `TestResult` schema reflecting the DTO above.
- Update every endpoint reference to the result schema.

After regenerating the OpenAPI doc, verify the Web UI's generation pipeline (`openapi-typescript`) consumes it without errors. If the project owns that pipeline, regenerate now and confirm the UI build is at least not broken at the type layer.

### 4.8 Threshold storage and evaluation

If thresholds were previously stored as `(metric_key, op, numeric_value)`:

- Migrate the `value` column to JSONB carrying the same `{type, value}` shape, so a threshold can compare against a typed value.
- Evaluation logic does typed comparison: numeric ops require numeric value on both sides; `EQ`/`NE` work for any matching type; throw a clear error if types don't match.

If thresholds weren't previously implemented, build the minimum spec'd above; do not invent richer functionality in this refactor — it can land in a follow-up.

### 4.9 Notifications

Update notification rules:

- "Test failed" trigger: `status ∈ {FAIL, ERROR, TIMEOUT}` AND `severity ∈ {ERROR, CRITICAL}`.
- "Test warning" trigger: `status ∈ {FAIL, ERROR}` AND `severity == WARNING`.
- "Run completed" trigger: run status moves to `completed`.
- "Run aborted" trigger: run status moves to `cancelled` and includes any results with `status == ABORTED`.

Notifications carry the result's `module_id`, `test_id`, `status`, `severity`, `error_message`, and a deep-link to the run detail.

### 4.10 Audit log

Audit entries that reference result fields update similarly: `passed` → `status`. Add new audit actions if needed: `result.threshold_crossed`, `run.aborted_with_partial_results`.

### 4.11 Metrics (Prometheus)

Update `/metrics` counters:

- `ivv_server_results_ingested_total{status="..."}` — labeled by status, replacing any boolean-labeled counter.
- `ivv_server_results_ingested_total{severity="..."}` — separate counter or combined label set.
- `ivv_server_run_terminal_total{status="completed|failed|cancelled"}` — unchanged but verify.

### 4.12 Logs

Search the Server source for log lines referencing old result fields (`passed`, `.data`, `duration<double>`, etc.). Replace with new fields. Maintain structured logging context (`run_id`, `module_id`, `test_id`).

### 4.13 Includes and headers

Audit includes; any header that used to reference the old types replaced with the regenerated proto headers.

---

## 5. Tests — required coverage

### 5.1 Migration test
- Apply the migration against a snapshot containing old-shape rows (a fixture).
- Verify `status` column is backfilled correctly for every existing row.
- Verify the constraint `status > 0` holds after backfill.

### 5.2 Repository tests
- Insert a `TestResult` with non-empty `reason`, `metrics`, `monitoring`, `event`, plus two-level-deep subtests. Read it back. Compare structurally.
- Insert results with every `Status` value. Query by status; expect correct filtering.
- Insert results with every `Severity` value. Query.
- Insert numerics in `metrics`. Run a trend query extracting a metric over time. Expect typed numeric results.
- Subtests round-trip via `parent_id`.

### 5.3 Service tests
- `RunService` per-module summary: cover PASS-only, FAIL, ERROR, TIMEOUT, mixed-with-SKIPPED, ABORTED.
- `ThresholdService`: a typed GT crossing fires; a typed mismatch raises a clear error.
- `BaselineService`: numeric drift computed correctly; status regression flagged.
- `CompareService`: every category (`unchanged / improved / regressed / drifted / disappeared / appeared`) covered with a fixture.
- `AnomalyService`: outlier in a stream of stable values flagged; stable stream not flagged.

### 5.4 gRPC handler tests
- Streaming handler ingests a sequence of results with mixed statuses, including one with `TEST_STATUS_UNSPECIFIED` — verify rejection.
- Backpressure: slow DB writer; verify reads stall, no drops, no OOM.
- Unknown `module_id` in incoming result: rejected.
- Subtests in a result: persisted recursively.

### 5.5 REST tests
- Every result-bearing endpoint returns the new DTO shape.
- `status` and `severity` are uppercase strings.
- `TypedValue` JSON shape preserved end-to-end.
- CSV export columns and order correct.
- NDJSON export produces one DTO per line, parsable.
- OpenAPI spec validates (use a schema validator); examples in OpenAPI match real responses.

### 5.6 End-to-end test
- Bring up Server + Postgres via docker-compose.
- Drive a mock gRPC client streaming results in each of the four modes, with each `Status` value occurring at least once across the suite.
- Verify: DB rows correct, REST returns correct DTOs, notifications fire for failure events, report generation succeeds, compare/baseline/trend endpoints return reasonable data.

### 5.7 Grep verification
After all changes:
```
grep -rn "\.passed\(\)" server/src/ server/tests/
grep -rn "set_passed\b" server/src/ server/tests/
grep -rn "duration<double>" server/src/ server/tests/
grep -rn "\boutput\.data\b" server/src/ server/tests/
```
Expect zero hits in active code.

---

## 6. Workflow — phases

Long-running task. No human gates. Write each phase report under `server_update/`.

### Phase 1 — Discovery
1. Confirm the HAL refactor and proto updates are in place: read `protos/common.proto`. If `TestStatus`, `TestSeverity`, `TypedValue`, `TestOutput`, `TestResult` are not present as defined upstream, **stop** and report.
2. Confirm the Client App has been updated (mark this as informational; the Server's update doesn't strictly block on it, but mismatched expectations across these two surfaces in production will cause failures).
3. Find every Server file that references old result fields (`passed`, `data` map, `duration<double>`, related setters/getters on generated proto types). List them.
4. List every gRPC handler, every controller, every repository, every service, every report template, every test file affected.
5. List existing DB migrations; identify what the next migration version number should be.
6. List every Prometheus metric whose label set references old shapes.

Write `server_update/01_discovery.md`.

### Phase 2 — Plan
A concrete edit plan in dependency order:
1. Proto regen.
2. DB migration drafted and reviewed.
3. Repository updates.
4. Service updates.
5. gRPC handler updates.
6. REST controller / DTO / OpenAPI updates.
7. Analytics services.
8. Reports, notifications, metrics, logs.
9. Tests.
10. Grep verification.

Write `server_update/02_plan.md`.

### Phase 3 — Proto regen and build verification
1. Regenerate C++ proto bindings.
2. Confirm the project still configures.
3. Comment out only the call sites that break compilation (do not fix them yet) so the rest of the build can proceed for inspection — or leave the breakage and proceed to Phase 4. Document choice.

Write `server_update/03_protos.md`.

### Phase 4 — DB migration
1. Author the SQL migration per Section 4.3.
2. Test it against a fixture DB with old-shape rows. Verify backfill.
3. Apply rollback (if reversible). Document if not.
4. Land the migration.

Write `server_update/04_migration.md`.

### Phase 5 — Repository updates
1. Update every repository method per Section 4.4.
2. Add the new methods (`get_subtests`, `find_module_trend`, etc.).
3. Add repository tests per Section 5.2. Run until green.

Write `server_update/05_repositories.md`.

### Phase 6 — Service updates
1. Update `ResultService`, `RunService`, `ThresholdService`, `AnomalyService`, `BaselineService`, `CompareService`, `ReportService` per Section 4.5.
2. Add service tests per Section 5.3.
3. Run service test layer until green.

Write `server_update/06_services.md`.

### Phase 7 — gRPC handlers
1. Update handlers per Section 4.2.
2. Add handler tests per Section 5.4.
3. Verify streaming backpressure preserved.

Write `server_update/07_grpc.md`.

### Phase 8 — REST, DTOs, OpenAPI
1. Update controllers and DTOs per Section 4.6.
2. Regenerate OpenAPI; validate against a schema validator.
3. Update CSV/NDJSON exports.
4. Add REST tests per Section 5.5.

Write `server_update/08_rest.md`.

### Phase 9 — Notifications, metrics, audit, logs
1. Update notification rules per Section 4.9.
2. Update Prometheus metrics per Section 4.11.
3. Update audit log actions per Section 4.10.
4. Update log lines per Section 4.12.

Write `server_update/09_observability.md`.

### Phase 10 — End-to-end verification
1. Build clean from scratch.
2. Run full unit + integration test suite.
3. Bring up via docker-compose; run the E2E test from Section 5.6.
4. Run grep verification per Section 5.7.

Write `server_update/10_verification.md` with evidence.

### Phase 11 — Final report
Summarize: every file changed, migration applied, fields exposed in REST, OpenAPI diff summary, test coverage delta, ambiguities resolved.

Write `server_update/11_final_report.md`.

---

## 7. Hard rules

- **`protos/common.proto` is the source of truth** for status/severity enum integer values. The DB migration encodes those integers; if the proto changes them later, the migration becomes wrong. Read the actual proto, hardcode the actual numbers, leave a comment with the version of the proto consulted.
- **Single results table.** No per-module tables. Typed maps in JSONB. Indexes per Section 4.3.
- **No `switch(module_id)`** introduced anywhere as a side effect.
- **Streaming handlers persist as they arrive.** No buffering whole streams to end-of-stream.
- **Backpressure preserved.** Bounded queues; the new larger payloads must not silently bypass size accounting.
- **The proto wire payload (`payload_bytes` + `payload_type_url`) is retained** alongside the projected structured columns. The structured columns are for querying; the bytes are for fidelity. Web UI can fall back to bytes if it needs a type the server didn't decode.
- **`status` and `severity` are uppercase strings on REST.** Never numeric. Document in OpenAPI.
- **Typed values keep their type in JSON.** `{type, value}` everywhere. No silent stringification.
- **CSV export is a defined contract** — column order frozen per Section 4.6. Document in README so downstream users don't break.
- **Migration is additive.** No DROP of `passed` in this migration. Schedule for later.
- **No HAL changes. No proto changes. No on-target Client changes** as a side effect of this Server update.
- **Audit anyone reading `passed`** — once Phase 10 grep is clean, treat that as the contract.
- **OpenAPI is the contract for the Web UI.** If the OpenAPI document is stale, the Web UI's generated client will silently break. Regenerate every time the controller layer changes.

---

## 8. Deliverables

At completion of Phase 11:
- Server builds clean.
- Postgres migration applied; old data backfilled.
- All unit + integration tests green.
- E2E with docker-compose green, covering every `Status` value.
- Grep for `\.passed\(\)`, `set_passed\b`, `duration<double>`, `output\.data` returns zero hits in active code.
- OpenAPI document regenerated and validates.
- CSV and NDJSON exports follow the documented contract.
- All phase reports under `server_update/01_…` through `server_update/11_final_report.md`.
- A short note in the README describing the new result shape, the JSONB encoding of `TypedValue`, the CSV column order, and the deprecation status of the `passed` column.

Begin with Phase 1.