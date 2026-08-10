-- Migration: 0002_runs
-- Creates test_configs, templates, and runs tables.

BEGIN;

-- ── Test Configurations ───────────────────────────────────────────────────────
-- Stores reusable test configurations. Each config specifies which modules to
-- test, what mode to use, and the per-module parameters.
CREATE TABLE IF NOT EXISTS test_configs (
    id               UUID        NOT NULL DEFAULT gen_random_uuid() PRIMARY KEY,
    name             TEXT        NOT NULL,
    description      TEXT        NOT NULL DEFAULT '',
    mode             TEXT        NOT NULL DEFAULT 'single_shot'
        CHECK (mode IN ('single_shot', 'long_term', 'monitoring', 'event_based')),
    common_config    JSONB       NOT NULL DEFAULT '{}',
    -- Array of: {module_id, enabled, frequency, monitoring_params,
    --            event_condition, module_config_payload (base64),
    --            module_config_type_url, notes}
    modules          JSONB       NOT NULL DEFAULT '[]',
    created_at       TIMESTAMPTZ NOT NULL DEFAULT NOW(),
    created_by       UUID        REFERENCES users(id) ON DELETE SET NULL,
    is_archived      BOOLEAN     NOT NULL DEFAULT FALSE
);

CREATE INDEX IF NOT EXISTS test_configs_created_by_idx ON test_configs (created_by);
CREATE INDEX IF NOT EXISTS test_configs_archived_idx   ON test_configs (is_archived);

-- ── Templates ─────────────────────────────────────────────────────────────────
-- Same shape as test_configs but are reusable blueprints (not directly run).
CREATE TABLE IF NOT EXISTS templates (
    id               UUID        NOT NULL DEFAULT gen_random_uuid() PRIMARY KEY,
    name             TEXT        NOT NULL,
    description      TEXT        NOT NULL DEFAULT '',
    mode             TEXT        NOT NULL DEFAULT 'single_shot'
        CHECK (mode IN ('single_shot', 'long_term', 'monitoring', 'event_based')),
    common_config    JSONB       NOT NULL DEFAULT '{}',
    modules          JSONB       NOT NULL DEFAULT '[]',
    created_at       TIMESTAMPTZ NOT NULL DEFAULT NOW(),
    created_by       UUID        REFERENCES users(id) ON DELETE SET NULL,
    is_archived      BOOLEAN     NOT NULL DEFAULT FALSE
);

CREATE INDEX IF NOT EXISTS templates_created_by_idx ON templates (created_by);
CREATE INDEX IF NOT EXISTS templates_archived_idx   ON templates (is_archived);

-- ── Runs ──────────────────────────────────────────────────────────────────────
CREATE TABLE IF NOT EXISTS runs (
    id                   UUID        NOT NULL DEFAULT gen_random_uuid() PRIMARY KEY,
    name                 TEXT        NOT NULL DEFAULT '',
    description          TEXT        NOT NULL DEFAULT '',
    -- Snapshot of the config at the time the run was started (frozen).
    test_config_snapshot JSONB       NOT NULL DEFAULT '{}',
    mode                 TEXT        NOT NULL DEFAULT 'single_shot',
    status               TEXT        NOT NULL DEFAULT 'queued'
        CHECK (status IN ('queued','running','paused','completed','failed','cancelled')),
    started_at           TIMESTAMPTZ,
    ended_at             TIMESTAMPTZ,
    error_message        TEXT,
    tags                 JSONB       NOT NULL DEFAULT '[]',
    notes                TEXT        NOT NULL DEFAULT '',
    baseline_flag        BOOLEAN     NOT NULL DEFAULT FALSE,
    -- Array of {id, filename, content_type, size_bytes, storage_path}
    attached_files       JSONB       NOT NULL DEFAULT '[]',
    created_at           TIMESTAMPTZ NOT NULL DEFAULT NOW(),
    created_by           UUID        REFERENCES users(id) ON DELETE SET NULL
);

CREATE INDEX IF NOT EXISTS runs_status_idx     ON runs (status);
CREATE INDEX IF NOT EXISTS runs_started_at_idx ON runs (started_at);
CREATE INDEX IF NOT EXISTS runs_created_by_idx ON runs (created_by);
CREATE INDEX IF NOT EXISTS runs_mode_idx       ON runs (mode);

-- At most one baseline run per mode (partial unique index).
CREATE UNIQUE INDEX IF NOT EXISTS runs_baseline_mode_idx
    ON runs (mode) WHERE baseline_flag = TRUE;

-- ── Schedules ─────────────────────────────────────────────────────────────────
CREATE TABLE IF NOT EXISTS schedules (
    id              UUID        NOT NULL DEFAULT gen_random_uuid() PRIMARY KEY,
    template_id     UUID        NOT NULL REFERENCES templates(id) ON DELETE CASCADE,
    -- cron expression (e.g. "0 */6 * * *") or NULL for one-time
    cron_expression TEXT,
    -- For one-time runs: exact timestamp to fire
    run_at          TIMESTAMPTZ,
    enabled         BOOLEAN     NOT NULL DEFAULT TRUE,
    last_run_at     TIMESTAMPTZ,
    next_run_at     TIMESTAMPTZ,
    created_at      TIMESTAMPTZ NOT NULL DEFAULT NOW(),
    created_by      UUID        REFERENCES users(id) ON DELETE SET NULL,
    CONSTRAINT schedule_type_check CHECK (
        (cron_expression IS NOT NULL AND run_at IS NULL) OR
        (cron_expression IS NULL     AND run_at IS NOT NULL)
    )
);

CREATE INDEX IF NOT EXISTS schedules_template_idx    ON schedules (template_id);
CREATE INDEX IF NOT EXISTS schedules_next_run_at_idx ON schedules (next_run_at) WHERE enabled = TRUE;

INSERT INTO schema_migrations (version) VALUES ('0002_runs')
    ON CONFLICT (version) DO NOTHING;

COMMIT;

-- DOWN:
-- DROP TABLE IF EXISTS schedules;
-- DROP TABLE IF EXISTS runs;
-- DROP TABLE IF EXISTS templates;
-- DROP TABLE IF EXISTS test_configs;
