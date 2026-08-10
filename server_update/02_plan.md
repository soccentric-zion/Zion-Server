# Phase 2 — Edit Plan _Generated: 2026-05-14_ Dependency-ordered list of changes. Each step is safe to apply before the next. --- ## Step 1 — CMakeLists: compile `client_service.proto` Add `client_service.proto` → `client_service.pb.{h,cc}` + `client_service.grpc.pb.{h,cc}` to
`zion-proto`. This gives `ClientService::Service` to implement. --- ## Step 2 — DB migration `0010_result_fields.sql` ```
RENAME status → status_legacy
ADD status SMALLINT NOT NULL DEFAULT 0
ADD severity SMALLINT NOT NULL DEFAULT 0
ADD test_id TEXT NOT NULL DEFAULT ''
ADD test_name TEXT NOT NULL DEFAULT ''
ADD sequence BIGINT NOT NULL DEFAULT 0
ADD duration_ns BIGINT NOT NULL DEFAULT 0
ADD reason JSONB NOT NULL DEFAULT '{}'::jsonb
ADD metrics JSONB NOT NULL DEFAULT '{}'::jsonb
ADD monitoring JSONB NOT NULL DEFAULT '{}'::jsonb
ADD event JSONB NOT NULL DEFAULT '{}'::jsonb
ADD parent_id UUID NULL REFERENCES results(id) ON DELETE CASCADE
Backfill status from status_legacy
Add constraint status > 0 after backfill
Add all indexes from spec
CREATE TABLE run_summaries(...)
``` --- ## Step 3 — `src/util/ResultConvert.h/.cc` New utility: `proto_to_result(TestResult, run_id, module_version, mode)` → `Result`.
Handles recursive subtests. Contains `typed_map_to_json`, `string_map_to_json`.
Contains `status_to_string(int16_t)` and `severity_to_string(int16_t)` used by REST. --- ## Step 4 — `src/repositories/ResultRepository.h/.cc` Update `Result` struct (new fields). Update `ResultFilter` (new filters).
Update `insert` to write all new columns + recursive subtests.
Update `list`/ `find_by_id` to read new columns.
Add: `find_last_per_module`, `find_module_trend`, `find_distribution`,
`find_for_compare`, `get_subtests`, `upsert_run_summary`. --- ## Step 5 — `src/services/RunService.h/.cc` Add `ingest_test_result(hal::common::TestResult, run_id, module_version, mode)`.
Add `is_terminal_failure(int16_t status, int16_t severity) → bool`.
Update per-module badge summary logic.
Update `complete_run` to derive run terminal status from result statuses. --- ## Step 6 — New analytics services - `ResultService` — wraps repository for analytics queries (trend, distribution)
- `ThresholdService` — per-`(module_id, test_id, metric_key)` threshold evaluation
- `AnomalyService` — z-score/ IQR over recent results of a metric
- `BaselineService` — baseline comparison, drift computation
- `CompareService` — two-run side-by-side comparison --- ## Step 7 — `src/services/NotificationService.cc` Update `emit` triggers:
- `test.failure` fires when `status ∈ {FAIL, ERROR, TIMEOUT}` AND `severity ∈ {ERROR, CRITICAL}`
- `test.warning` fires when `status ∈ {FAIL, ERROR}` AND `severity == WARNING`
- `run.aborted` fires when run transitions to cancelled with `ABORTED` results --- ## Step 8 — `src/grpc_server/HalServiceImpl.cc` Update `persist_result`: unpack `ResultEnvelope.payload` as `hal::common::TestResult`.
Reject `TEST_STATUS_UNSPECIFIED`. Validate `module_id` from `TestResult`. --- ## Step 9 — `src/grpc_server/ClientServiceImpl.h/.cc` New: implements `zion::stream::ClientService::Service::Connect`.
Bidi stream: reads `ClientMessage`, dispatches `TestResultBatch` to DB via BoundedQueue.
Writes `ServerMessage` (BatchAck, LogBatchAck) back.
Backpressure: BoundedQueue(256); if full, reader stalls. --- ## Step 10 — `src/grpc_server/GrpcServerRunner.h/.cc` Add `ClientServiceImpl` instance. Register it in `BuildAndStart`. --- ## Step 11 — `src/rest_api/controllers/RunsController.cc` Update `result_to_json` to output new DTO:
`status`/`severity` as uppercase strings, typed maps, subtests array, `duration_ns`, etc.
Update CSV export: new column order per spec.
Update NDJSON export (one result per line).
Update report generation: status string + severity in HTML. --- ## Step 12 — `src/rest_api/controllers/ModulesController.h/.cc` Add endpoints:
- `GET/api/v1/modules/{id}/trend?test_id=&field=&from=&to=`
- `GET/api/v1/modules/{id}/distribution?test_id=&field=&runs=`
- `GET/api/v1/modules/{id}/heatmap?test_id=&field=&from=&to=` --- ## Step 13 — `src/rest_api/DrogonAppFactory.h/.cc` + `src/main.cpp` Register new services (`ResultService`, `ThresholdService`, `AnomalyService`,
`BaselineService`, `CompareService`) in the DI wiring.
Register `ClientServiceImpl` in gRPC server. --- ## Step 14 — `src/orchestration/RunManager.cc` Update `on_result` to call `run_svc_.ingest_test_result(...)` with full proto. --- ## Step 15 — Tests See Phase 10. --- ## Step 16 — Grep verification ```bash
grep -rn "\.passed" src/ tests/
grep -rn "set_passed\b" src/ tests/
grep -rn '"pass"\|== "fail"\|== "error"\|== "unknown"' src/ tests/
grep -rn "status_legacy\b" src/ # should only appear in migration
``` Expected: zero hits in active code (except migration file + status_legacy backcompat in ResultConvert).
