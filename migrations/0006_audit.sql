-- Migration: 0006_audit
-- Creates the audit_log table.

BEGIN;

CREATE TABLE IF NOT EXISTS audit_log (
    id          UUID        NOT NULL DEFAULT gen_random_uuid() PRIMARY KEY,
    -- NULL means a system-initiated action (scheduler, heartbeat, etc.)
    user_id     UUID        REFERENCES users(id) ON DELETE SET NULL,
    -- Verb: e.g. "run.started", "config.created", "auth.login", "ota.upgrade.triggered"
    action      TEXT        NOT NULL,
    -- The type of entity being acted upon: run | config | board | ota_upgrade | user | ...
    target_type TEXT        NOT NULL DEFAULT '',
    -- UUID of the target entity (as text to avoid FK constraints on heterogeneous tables).
    target_id   TEXT        NOT NULL DEFAULT '',
    -- Additional context (request IP, diff, old/new values, etc.)
    metadata    JSONB       NOT NULL DEFAULT '{}',
    created_at  TIMESTAMPTZ NOT NULL DEFAULT NOW()
);

CREATE INDEX IF NOT EXISTS audit_log_user_idx       ON audit_log (user_id) WHERE user_id IS NOT NULL;
CREATE INDEX IF NOT EXISTS audit_log_created_at_idx ON audit_log (created_at DESC);
CREATE INDEX IF NOT EXISTS audit_log_action_idx     ON audit_log (action);
CREATE INDEX IF NOT EXISTS audit_log_target_idx     ON audit_log (target_type, target_id);

INSERT INTO schema_migrations (version) VALUES ('0006_audit')
    ON CONFLICT (version) DO NOTHING;

COMMIT;

-- DOWN:
-- DROP TABLE IF EXISTS audit_log;
