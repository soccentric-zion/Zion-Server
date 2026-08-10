# Phase 1 — Discovery _Generated: 2026-05-14_ --- ## 1. Proto verification `Zion-HAL/protos/common.proto` confirms the following types are present and correctly defined: | Symbol | Status |
|--------|--------|
| `TestStatus` (enum, 8 values) | ✅ present |
| `TestSeverity` (enum, 5 values) | ✅ present |
| `TypedValue` (message, oneof bool/int64/double/string) | ✅ present |
| `TestOutput` (message, reserved 1–2 for old `passed`/`data`) | ✅ present |
| `TestResult` (message, with subtests) | ✅ present |
| `ResultEnvelope` (updated: `status` uses `TestStatus`) | ✅ present | **Proto enum integers (canonical — pinned to `common.proto` HEAD):** ```
TestStatus: TEST_STATUS_UNSPECIFIED = 0 TEST_STATUS_PASS = 1 TEST_STATUS_FAIL = 2 TEST_STATUS_ERROR = 3 TEST_STATUS_TIMEOUT = 4 TEST_STATUS_ABORTED = 5 (6 = PARTIAL — deprecated alias, do not produce) TEST_STATUS_SKIPPED = 7 TestSeverity: TEST_SEVERITY_UNSPECIFIED = 0 TEST_SEVERITY_INFO = 1 TEST_SEVERITY_WARNING = 2 TEST_SEVERITY_ERROR = 3 TEST_SEVERITY_CRITICAL = 4
``` `client_service.proto` is present and defines `ClientService::Connect` (bidi stream). The updated
Client App packs `hal.common.TestResult` into `ResultSample.payload` with
`type_url = "type.googleapis.com/hal.common.TestResult"`. This is confirmed in
`Zion-Client/src/run/RunOrchestrator.cc`. --- ## 2. Client App status The Client App has been updated. `RunOrchestrator::flush_results` stamps `sequence`, `timestamp`,
packs `hal::common::TestResult` into every `ResultSample`, and sets `type_url` to
`"type.googleapis.com/hal.common.TestResult"`. The Server must consume this shape. --- ## 3. Old result fields in Server source Files referencing **old** result shape: | File | Old reference |
|------|--------------|
| `src/grpc_server/HalServiceImpl.cc` | `env.status == hal::common::PASS ? "pass" : "fail"` |
| `src/repositories/ResultRepository.h` | `std::string status` (string "pass/fail/error/unknown") |
| `src/repositories/ResultRepository.cc` | INSERT and SELECT use old string `status` column |
| `src/services/RunService.cc` | `r.status == "pass"` check in report |
| `src/rest_api/controllers/RunsController.cc` | `r.status` as plain string in JSON; old CSV columns |
| `src/rest_api/controllers/RunsController.cc` | `if (r.status == "pass")` in report generation |
| `src/orchestration/RunManager.cc` | `on_result` takes old `Result` struct | Old DB schema (`migrations/0003_results.sql`): `status TEXT CHECK (status IN ('pass','fail','error','unknown'))`.
No `passed BOOLEAN` column (schema used string status instead). The "old boolean" equivalent is the string `status`. --- ## 4. Affected files summary ### gRPC handlers
- `src/grpc_server/HalServiceImpl.h`/ `.cc` — `persist_result` needs TestResult unpack
- `src/grpc_server/GrpcServerRunner.h`/ `.cc` — needs `ClientServiceImpl` registered
- **New**: `src/grpc_server/ClientServiceImpl.h`/ `.cc` — implements `ClientService::Connect` ### Repositories
- `src/repositories/ResultRepository.h`/ `.cc` — new struct, new methods ### Services
- `src/services/RunService.h`/ `.cc` — `ingest_test_result`, `is_terminal_failure`
- **New**: `src/services/ResultService.h`/ `.cc` — analytics facade
- **New**: `src/services/ThresholdService.h`/ `.cc`
- **New**: `src/services/AnomalyService.h`/ `.cc`
- **New**: `src/services/BaselineService.h`/ `.cc`
- **New**: `src/services/CompareService.h`/ `.cc`
- `src/services/NotificationService.cc` — updated trigger rules ### REST controllers
- `src/rest_api/controllers/RunsController.cc` — new result DTO, CSV export
- `src/rest_api/controllers/ModulesController.h`/ `.cc` — trend/distribution/heatmap endpoints
- `src/rest_api/DrogonAppFactory.h`/ `.cc` — register new services
- `src/main.cpp` — wire new services + ClientService ### Utilities
- **New**: `src/util/ResultConvert.h`/ `.cc` — proto ↔ Result struct conversion + TypedValue JSON ### Orchestration
- `src/orchestration/RunManager.h`/ `.cc` — update `on_result` signature ### Migrations
- **New**: `migrations/0010_result_fields.sql` ### CMakeLists
- `CMakeLists.txt` — add `client_service.proto` compilation ### Tests
- **New**: `tests/unit/repositories/ResultRepositoryTest.cc`
- **New**: `tests/unit/services/RunServiceTest.cc`
- **New**: `tests/unit/services/ThresholdServiceTest.cc`
- **New**: `tests/unit/grpc/ClientServiceImplTest.cc` --- ## 5. Existing DB migrations ```
0001_init.sql
0002_runs.sql
0003_results.sql ← results table with old TEXT status
0004_logs.sql
0005_ota.sql
0006_audit.sql
0007_notifications.sql
0008_alerts.sql
0009_app_settings.sql
``` **Next migration number: `0010`** → `migrations/0010_result_fields.sql` --- ## 6. Prometheus metrics referencing old shapes No existing Prometheus integration found in the source (the `prometheus-cpp` library is optional and gated behind `ZION_HAS_PROMETHEUS`). No existing per-result counters were found. New counters will be added in Phase 9. --- ## 7. Findings requiring attention - The `HalService` gRPC server-streaming model in `HalServiceImpl` uses a stub "echo back PASS and persist" implementation. The real ingest path is `ClientService` bidi stream. `HalService` handlers will be updated to properly unpack `TestResult` from `ResultEnvelope.payload` for the single-shot case; the streaming cases will delegate to `ClientServiceImpl` patterns.
- The `status TEXT` column must be renamed to `status_legacy TEXT` before the new `status SMALLINT` column can be added (same column name, different type).
- No `run_summaries` table exists yet; it will be created in migration 0010.
