-- Migration: 0003_results
-- Creates the results table (all module payloads stored as BYTEA + type_url).

BEGIN;

CREATE TABLE IF NOT EXISTS results (
    id               UUID        NOT NULL DEFAULT gen_random_uuid() PRIMARY KEY,
    run_id           UUID        NOT NULL REFERENCES runs(id) ON DELETE CASCADE,
    module_id        TEXT        NOT NULL,
    module_version   TEXT        NOT NULL DEFAULT '',
    timestamp        TIMESTAMPTZ NOT NULL DEFAULT NOW(),
    mode             TEXT        NOT NULL DEFAULT 'single_shot',
    status           TEXT        NOT NULL DEFAULT 'pass'
        CHECK (status IN ('pass', 'fail', 'error', 'unknown')),
    error_message    TEXT,
    -- Raw serialized proto bytes of the module-specific result message.
    payload_bytes    BYTEA,
    -- google.protobuf.Any type_url, e.g. "type.googleapis.com/zion.CpuResult"
    payload_type_url TEXT        NOT NULL DEFAULT ''
);

-- Primary lookup: all results for a run, ordered by time.
CREATE INDEX IF NOT EXISTS results_run_ts_idx
    ON results (run_id, timestamp);

-- Secondary: per-module trend queries.
CREATE INDEX IF NOT EXISTS results_module_ts_idx
    ON results (module_id, timestamp);

-- Status filter.
CREATE INDEX IF NOT EXISTS results_status_idx
    ON results (status);

INSERT INTO schema_migrations (version) VALUES ('0003_results')
    ON CONFLICT (version) DO NOTHING;

COMMIT;

-- DOWN:
-- DROP TABLE IF EXISTS results;
