-- Migration 0009: app_settings key-value store
-- Used by SettingsController to persist runtime-mutable settings such as
-- data retention policy without requiring a server restart.

CREATE TABLE IF NOT EXISTS app_settings (
    key         TEXT        PRIMARY KEY,
    value       TEXT        NOT NULL,
    updated_at  TIMESTAMPTZ NOT NULL DEFAULT NOW()
);

-- Seed defaults
INSERT INTO app_settings (key, value)
VALUES
    ('retention_days',      '90'),
    ('max_run_results',     '0')   -- 0 = unlimited
ON CONFLICT (key) DO NOTHING;
