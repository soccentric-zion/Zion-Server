-- Migration: 0004_logs
-- Creates log_entries, log_bookmarks, and saved_log_views tables.

BEGIN;

CREATE TABLE IF NOT EXISTS log_entries (
    id         UUID        NOT NULL DEFAULT gen_random_uuid() PRIMARY KEY,
    -- Source tag: kernel | syslog | app | grpc | http | ...
    source     TEXT        NOT NULL DEFAULT 'app',
    severity   TEXT        NOT NULL DEFAULT 'info'
        CHECK (severity IN ('trace', 'debug', 'info', 'warn', 'error', 'critical')),
    timestamp  TIMESTAMPTZ NOT NULL DEFAULT NOW(),
    message    TEXT        NOT NULL DEFAULT '',
    -- Arbitrary structured metadata (module_id, run_id, pid, etc.)
    metadata   JSONB       NOT NULL DEFAULT '{}',
    -- Optional correlation with a run.
    run_id     UUID        REFERENCES runs(id) ON DELETE SET NULL
);

-- Primary time-series index.
CREATE INDEX IF NOT EXISTS log_entries_ts_source_idx
    ON log_entries (timestamp DESC, source);

-- Correlation index.
CREATE INDEX IF NOT EXISTS log_entries_run_idx
    ON log_entries (run_id) WHERE run_id IS NOT NULL;

-- Severity filter.
CREATE INDEX IF NOT EXISTS log_entries_severity_idx
    ON log_entries (severity, timestamp DESC);

-- Full-text search on message.
CREATE INDEX IF NOT EXISTS log_entries_fts_idx
    ON log_entries USING gin (to_tsvector('english', message));

-- ── Log Bookmarks ─────────────────────────────────────────────────────────────
CREATE TABLE IF NOT EXISTS log_bookmarks (
    id         UUID        NOT NULL DEFAULT gen_random_uuid() PRIMARY KEY,
    user_id    UUID        NOT NULL REFERENCES users(id) ON DELETE CASCADE,
    log_id     UUID        NOT NULL REFERENCES log_entries(id) ON DELETE CASCADE,
    note       TEXT        NOT NULL DEFAULT '',
    created_at TIMESTAMPTZ NOT NULL DEFAULT NOW()
);

CREATE UNIQUE INDEX IF NOT EXISTS log_bookmarks_user_log_idx
    ON log_bookmarks (user_id, log_id);

-- ── Saved Log Views (filter presets) ─────────────────────────────────────────
CREATE TABLE IF NOT EXISTS saved_log_views (
    id         UUID        NOT NULL DEFAULT gen_random_uuid() PRIMARY KEY,
    user_id    UUID        NOT NULL REFERENCES users(id) ON DELETE CASCADE,
    name       TEXT        NOT NULL,
    -- JSON-serialized filter state: {source, severity, from, to, run_id, query}
    filters    JSONB       NOT NULL DEFAULT '{}',
    created_at TIMESTAMPTZ NOT NULL DEFAULT NOW()
);

CREATE INDEX IF NOT EXISTS saved_log_views_user_idx ON saved_log_views (user_id);

INSERT INTO schema_migrations (version) VALUES ('0004_logs')
    ON CONFLICT (version) DO NOTHING;

COMMIT;

-- DOWN:
-- DROP TABLE IF EXISTS saved_log_views;
-- DROP TABLE IF EXISTS log_bookmarks;
-- DROP TABLE IF EXISTS log_entries;
