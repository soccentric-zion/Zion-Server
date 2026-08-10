-- Migration: 0010_result_fields
-- Adds new columns to `results` for the refactored HAL TestResult / TestOutput shape.
-- Adds `run_summaries` table for efficient per-module last-status lookups.
--
-- Proto integer mappings consulted from Zion-HAL/protos/common.proto (HEAD, 2026-05-14):
--
--   TestStatus:
--     0 = TEST_STATUS_UNSPECIFIED (never stored; constraint below prevents this)
--     1 = TEST_STATUS_PASS
--     2 = TEST_STATUS_FAIL
--     3 = TEST_STATUS_ERROR
--     4 = TEST_STATUS_TIMEOUT
--     5 = TEST_STATUS_ABORTED
--     6 = PARTIAL (deprecated alias; do not produce)
--     7 = TEST_STATUS_SKIPPED
--
--   TestSeverity:
--     0 = TEST_SEVERITY_UNSPECIFIED
--     1 = TEST_SEVERITY_INFO
--     2 = TEST_SEVERITY_WARNING
--     3 = TEST_SEVERITY_ERROR
--     4 = TEST_SEVERITY_CRITICAL
--
-- TypedValue JSON encoding (used in metrics, monitoring, event columns):
--   { "type": "bool",   "value": true  }
--   { "type": "int",    "value": 1234  }
--   { "type": "double", "value": 42.5  }
--   { "type": "string", "value": "hi"  }
-- The top-level JSONB object is: { key → TypedValue object }

BEGIN;

-- ── Step 1: rename the old TEXT status column ────────────────────────────────
-- Kept for backward compatibility and backfill purposes.  Schedule DROP in a
-- future migration once all readers have been updated.
ALTER TABLE results RENAME COLUMN status TO status_legacy;

COMMENT ON COLUMN results.status_legacy IS
  'DEPRECATED — old string status (pass/fail/error/unknown). '
  'Use the new status SMALLINT column. Scheduled for removal in a future migration.';

-- ── Step 2: add new structured columns ──────────────────────────────────────
ALTER TABLE results
    -- status: TestStatus enum integer (see mapping above); 0 = unspecified (disallowed after backfill)
    ADD COLUMN status        SMALLINT      NOT NULL DEFAULT 0,
    -- severity: TestSeverity enum integer (see mapping above)
    ADD COLUMN severity      SMALLINT      NOT NULL DEFAULT 0,
    -- test_id: hal::TestResult.test_id (identifies which test within the module)
    ADD COLUMN test_id       TEXT          NOT NULL DEFAULT '',
    -- test_name: hal::TestResult.name (human-readable label)
    ADD COLUMN test_name     TEXT          NOT NULL DEFAULT '',
    -- sequence: hal::TestResult.sequence (monotonic within a run per module)
    ADD COLUMN sequence      BIGINT        NOT NULL DEFAULT 0,
    -- duration_ns: hal::TestResult.duration expressed in nanoseconds
    ADD COLUMN duration_ns   BIGINT        NOT NULL DEFAULT 0,
    -- reason: map<string,string> from TestOutput.reason (human-readable explanation)
    ADD COLUMN reason        JSONB         NOT NULL DEFAULT '{}'::jsonb,
    -- metrics: map<string,TypedValue> from TestOutput.metrics (measured values)
    ADD COLUMN metrics       JSONB         NOT NULL DEFAULT '{}'::jsonb,
    -- monitoring: map<string,TypedValue> from TestOutput.monitoring (env variables)
    ADD COLUMN monitoring    JSONB         NOT NULL DEFAULT '{}'::jsonb,
    -- event: map<string,TypedValue> from TestOutput.event (event payload)
    ADD COLUMN event         JSONB         NOT NULL DEFAULT '{}'::jsonb,
    -- parent_id: UUID of parent TestResult for subtests; NULL for top-level results
    ADD COLUMN parent_id     UUID          NULL REFERENCES results(id) ON DELETE CASCADE;

-- ── Step 3: backfill `status` from `status_legacy` ───────────────────────────
-- Mapping: 'pass' → 1, 'fail' → 2, 'error' → 3, 'unknown' → 2 (closest: FAIL)
UPDATE results
SET status = CASE status_legacy
    WHEN 'pass'    THEN 1   -- TEST_STATUS_PASS
    WHEN 'fail'    THEN 2   -- TEST_STATUS_FAIL
    WHEN 'error'   THEN 3   -- TEST_STATUS_ERROR
    WHEN 'unknown' THEN 2   -- map unknown → FAIL (conservative)
    ELSE                2   -- any other legacy value → FAIL
END;

-- ── Step 4: enforce that status is never UNSPECIFIED on stored rows ──────────
-- Only apply after backfill so all rows satisfy the constraint.
ALTER TABLE results
    ADD CONSTRAINT results_status_set CHECK (status > 0);

-- ── Step 5: drop old status_idx and add new indexes ─────────────────────────
DROP INDEX IF EXISTS results_status_idx;

CREATE INDEX IF NOT EXISTS results_status_ts_idx
    ON results (status, timestamp DESC);

CREATE INDEX IF NOT EXISTS results_module_test_ts_idx
    ON results (module_id, test_id, timestamp DESC);

CREATE INDEX IF NOT EXISTS results_run_sequence_idx
    ON results (run_id, module_id, sequence);

CREATE INDEX IF NOT EXISTS results_parent_idx
    ON results (parent_id)
    WHERE parent_id IS NOT NULL;

CREATE INDEX IF NOT EXISTS results_metrics_gin_idx
    ON results USING GIN (metrics);

CREATE INDEX IF NOT EXISTS results_monitoring_gin_idx
    ON results USING GIN (monitoring);

-- ── Step 6: run_summaries table ──────────────────────────────────────────────
-- Stores the most-recent result status per (run_id, module_id, test_id).
-- Upserted by ResultRepository::upsert_run_summary() after each result insert.
-- Enables O(1) per-module badge lookups on run detail page without window functions.
-- Refresh strategy: synchronous upsert in the same transaction as result insert.
CREATE TABLE IF NOT EXISTS run_summaries (
    run_id          UUID         NOT NULL REFERENCES runs(id) ON DELETE CASCADE,
    module_id       TEXT         NOT NULL,
    test_id         TEXT         NOT NULL DEFAULT '',
    last_status     SMALLINT     NOT NULL DEFAULT 0,
    last_severity   SMALLINT     NOT NULL DEFAULT 0,
    last_timestamp  TIMESTAMPTZ  NOT NULL DEFAULT NOW(),
    result_count    INT          NOT NULL DEFAULT 0,
    PRIMARY KEY (run_id, module_id, test_id)
);

COMMENT ON TABLE run_summaries IS
  'Per-(run,module,test) latest result summary. Upserted on every result insert. '
  'Enables fast badge lookups without window functions over the full results table.';

CREATE INDEX IF NOT EXISTS run_summaries_run_idx
    ON run_summaries (run_id);

-- ── Step 7: backfill run_summaries from existing results ─────────────────────
INSERT INTO run_summaries (run_id, module_id, test_id, last_status, last_severity, last_timestamp, result_count)
SELECT
    run_id,
    module_id,
    '' AS test_id,
    (array_agg(status ORDER BY timestamp DESC))[1] AS last_status,
    0 AS last_severity,
    MAX(timestamp) AS last_timestamp,
    COUNT(*) AS result_count
FROM results
WHERE parent_id IS NULL
GROUP BY run_id, module_id
ON CONFLICT (run_id, module_id, test_id) DO UPDATE
    SET last_status    = EXCLUDED.last_status,
        last_severity  = EXCLUDED.last_severity,
        last_timestamp = EXCLUDED.last_timestamp,
        result_count   = EXCLUDED.result_count;

-- ── Step 8: thresholds table ─────────────────────────────────────────────────
-- Stores per-(module_id, test_id, metric_key) threshold rules.
-- value_json uses the same TypedValue encoding as the metrics column.
CREATE TABLE IF NOT EXISTS thresholds (
    id           UUID     NOT NULL DEFAULT gen_random_uuid() PRIMARY KEY,
    module_id    TEXT     NOT NULL,
    test_id      TEXT     NOT NULL DEFAULT '',
    metric_key   TEXT     NOT NULL,
    op           TEXT     NOT NULL CHECK (op IN ('GT','LT','GE','LE','EQ','NE')),
    value_json   JSONB    NOT NULL,  -- { "type": "double", "value": 80.0 }
    created_at   TIMESTAMPTZ NOT NULL DEFAULT NOW(),
    updated_at   TIMESTAMPTZ NOT NULL DEFAULT NOW(),
    UNIQUE (module_id, test_id, metric_key, op)
);

CREATE INDEX IF NOT EXISTS thresholds_module_test_idx
    ON thresholds (module_id, test_id);

INSERT INTO schema_migrations (version) VALUES ('0010_result_fields')
    ON CONFLICT (version) DO NOTHING;

COMMIT;

-- DOWN (for reference — not auto-applied):
-- ALTER TABLE results DROP CONSTRAINT IF EXISTS results_status_set;
-- ALTER TABLE results DROP COLUMN IF EXISTS parent_id;
-- ALTER TABLE results DROP COLUMN IF EXISTS event;
-- ALTER TABLE results DROP COLUMN IF EXISTS monitoring;
-- ALTER TABLE results DROP COLUMN IF EXISTS metrics;
-- ALTER TABLE results DROP COLUMN IF EXISTS reason;
-- ALTER TABLE results DROP COLUMN IF EXISTS duration_ns;
-- ALTER TABLE results DROP COLUMN IF EXISTS sequence;
-- ALTER TABLE results DROP COLUMN IF EXISTS test_name;
-- ALTER TABLE results DROP COLUMN IF EXISTS test_id;
-- ALTER TABLE results DROP COLUMN IF EXISTS severity;
-- ALTER TABLE results DROP COLUMN IF EXISTS status;
-- ALTER TABLE results RENAME COLUMN status_legacy TO status;
-- DROP TABLE IF EXISTS run_summaries;
-- DROP TABLE IF EXISTS thresholds;
