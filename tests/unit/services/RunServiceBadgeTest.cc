// tests/unit/services/RunServiceBadgeTest.cc — unit tests for RunService badge logic.
#include "services/RunService.h"
#include <gtest/gtest.h>

using zion::RunService;
using zion::RunModuleSummary;

// ── is_terminal_failure ───────────────────────────────────────────────────────

TEST(RunServiceTest, TerminalFailure_FailWithCritical) {
    // FAIL(2) + CRITICAL(4) → failure
    EXPECT_TRUE(RunService::is_terminal_failure(2, 4));
}

TEST(RunServiceTest, TerminalFailure_ErrorWithError) {
    // ERROR(3) + ERROR(3) → failure
    EXPECT_TRUE(RunService::is_terminal_failure(3, 3));
}

TEST(RunServiceTest, TerminalFailure_TimeoutWithCritical) {
    // TIMEOUT(4) + CRITICAL(4) → failure
    EXPECT_TRUE(RunService::is_terminal_failure(4, 4));
}

TEST(RunServiceTest, NotTerminalFailure_PassWithCritical) {
    // PASS(1) even with CRITICAL severity → not a failure notification
    EXPECT_FALSE(RunService::is_terminal_failure(1, 4));
}

TEST(RunServiceTest, NotTerminalFailure_FailWithWarning) {
    // FAIL(2) + WARNING(2) → not a terminal failure (is_warning instead)
    EXPECT_FALSE(RunService::is_terminal_failure(2, 2));
}

TEST(RunServiceTest, NotTerminalFailure_SkippedWithCritical) {
    // SKIPPED(7) + CRITICAL(4) → not a failure
    EXPECT_FALSE(RunService::is_terminal_failure(7, 4));
}

TEST(RunServiceTest, NotTerminalFailure_AbortedWithCritical) {
    // ABORTED(5) + CRITICAL(4) — ABORTED does not count as terminal failure
    EXPECT_FALSE(RunService::is_terminal_failure(5, 4));
}

// ── is_warning ───────────────────────────────────────────────────────────────

TEST(RunServiceTest, Warning_FailWithWarning) {
    EXPECT_TRUE(RunService::is_warning(2, 2));
}

TEST(RunServiceTest, Warning_ErrorWithWarning) {
    EXPECT_TRUE(RunService::is_warning(3, 2));
}

TEST(RunServiceTest, NotWarning_PassWithWarning) {
    EXPECT_FALSE(RunService::is_warning(1, 2));
}

TEST(RunServiceTest, NotWarning_FailWithInfo) {
    EXPECT_FALSE(RunService::is_warning(2, 1));
}

// ── module_badge ─────────────────────────────────────────────────────────────

TEST(RunServiceTest, Badge_NoResults_ReturnsNever) {
    RunModuleSummary s;
    s.result_count = 0;
    s.last_status  = 0;
    EXPECT_EQ(RunService::module_badge(s, true), "NEVER");
}

TEST(RunServiceTest, Badge_NotEnabled_ReturnsNever) {
    RunModuleSummary s;
    s.result_count = 5;
    s.last_status  = 1;
    EXPECT_EQ(RunService::module_badge(s, false), "NEVER");
}

TEST(RunServiceTest, Badge_Pass) {
    RunModuleSummary s;
    s.result_count = 3;
    s.last_status  = 1;  // PASS
    EXPECT_EQ(RunService::module_badge(s, true), "PASS");
}

TEST(RunServiceTest, Badge_Fail) {
    RunModuleSummary s;
    s.result_count = 3;
    s.last_status  = 2;  // FAIL
    EXPECT_EQ(RunService::module_badge(s, true), "FAIL");
}

TEST(RunServiceTest, Badge_Error_TreatedAsFail) {
    RunModuleSummary s;
    s.result_count = 1;
    s.last_status  = 3;  // ERROR → FAIL badge
    EXPECT_EQ(RunService::module_badge(s, true), "FAIL");
}

TEST(RunServiceTest, Badge_Timeout_TreatedAsFail) {
    RunModuleSummary s;
    s.result_count = 1;
    s.last_status  = 4;  // TIMEOUT → FAIL badge
    EXPECT_EQ(RunService::module_badge(s, true), "FAIL");
}

TEST(RunServiceTest, Badge_Aborted) {
    RunModuleSummary s;
    s.result_count = 2;
    s.last_status  = 5;  // ABORTED
    EXPECT_EQ(RunService::module_badge(s, true), "ABORTED");
}

TEST(RunServiceTest, Badge_Skipped_ReturnsNever) {
    RunModuleSummary s;
    s.result_count = 1;
    s.last_status  = 7;  // SKIPPED → NEVER
    EXPECT_EQ(RunService::module_badge(s, true), "NEVER");
}
