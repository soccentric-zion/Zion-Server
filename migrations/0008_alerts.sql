-- Migration 0008: alert_rules table for threshold-based alerting
-- Each rule monitors a specific module field and fires when the threshold is crossed.

CREATE TABLE IF NOT EXISTS alert_rules (
    id           UUID        PRIMARY KEY DEFAULT gen_random_uuid(),
    module_id    TEXT        NOT NULL,
    field_path   TEXT        NOT NULL,
    operator     TEXT        NOT NULL CHECK (operator IN ('>', '<', '>=', '<=', '==')),
    threshold    FLOAT       NOT NULL,
    severity     TEXT        NOT NULL DEFAULT 'warning' CHECK (severity IN ('info', 'warning', 'critical')),
    enabled      BOOLEAN     NOT NULL DEFAULT TRUE,
    created_by   UUID        REFERENCES users(id) ON DELETE SET NULL,
    created_at   TIMESTAMPTZ NOT NULL DEFAULT NOW(),
    updated_at   TIMESTAMPTZ NOT NULL DEFAULT NOW()
);

CREATE INDEX IF NOT EXISTS alert_rules_module_idx ON alert_rules (module_id);
CREATE INDEX IF NOT EXISTS alert_rules_enabled_idx ON alert_rules (enabled) WHERE enabled = TRUE;
