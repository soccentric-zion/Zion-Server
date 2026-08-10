#pragma once
// services/CompareService.h — Compare two runs at the (module_id, test_id) granularity.
//
// Classification rules (applied in order):
//   appeared    — exists in run_b but not in run_a.
//   disappeared — exists in run_a but not in run_b.
//   regressed   — status worsened (numeric status value increased between runs).
//   improved    — status improved (numeric status value decreased).
//   unchanged   — same status, severity, and no significant metric drift.
//   drifted     — same status/severity but ≥1 metric changed by > drift_pct_threshold%.

#include "repositories/ResultRepository.h"
#include <string>
#include <vector>

namespace zion {

class CompareService {
public:
    // Returns a labelled comparison between run_a and run_b.
    // drift_pct_threshold: percentage change in a numeric metric that counts as "drifted".
    std::vector<CompareEntry> compare(const std::string& run_a,
                                       const std::string& run_b,
                                       double drift_pct_threshold = 5.0);

private:
    ResultRepository result_repo_;

    // Classify a single pair of entries.
    static std::string classify(const CompareEntry& raw,
                                 double drift_pct_threshold);
};

}  // namespace zion
