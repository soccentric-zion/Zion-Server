#pragma once
// services/AnomalyService.h — Z-score based anomaly detection over metric time-series.
//
// The service does NOT persist anomaly flags to the DB by default (stateless query).
// Callers get a boolean `is_anomaly` flag per TrendPoint in the returned sequence.

#include "repositories/ResultRepository.h"
#include <string>
#include <vector>

namespace zion {

struct AnomalyPoint {
    TrendPoint  point;
    bool        is_anomaly = false;
    double      z_score    = 0.0;
};

class AnomalyService {
public:
    // Returns recent N TrendPoints for (module_id, test_id, field), with z-score
    // anomaly flags.  Only numeric TypedValue variants participate.
    // window_size: number of recent results to include (default 100).
    // z_threshold: |z| > this → anomaly (default 3.0).
    std::vector<AnomalyPoint> detect(const std::string& module_id,
                                      const std::string& test_id,
                                      const std::string& metric_key,
                                      const std::string& from_ts,
                                      const std::string& to_ts,
                                      double z_threshold = 3.0);

private:
    ResultRepository result_repo_;
};

}  // namespace zion
