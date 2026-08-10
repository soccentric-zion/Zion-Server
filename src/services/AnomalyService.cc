#include "services/AnomalyService.h"

#include <algorithm>
#include <cmath>
#include <numeric>

namespace zion {

std::vector<AnomalyPoint> AnomalyService::detect(const std::string& module_id,
                                                   const std::string& test_id,
                                                   const std::string& metric_key,
                                                   const std::string& from_ts,
                                                   const std::string& to_ts,
                                                   double z_threshold) {
    auto points = result_repo_.find_module_trend(module_id, test_id, metric_key,
                                                  from_ts, to_ts);
    if (points.empty()) return {};

    // Collect numeric values.
    std::vector<double> vals;
    vals.reserve(points.size());
    for (const auto& p : points) vals.push_back(p.value);

    double n   = static_cast<double>(vals.size());
    double sum = std::accumulate(vals.begin(), vals.end(), 0.0);
    double mean = sum / n;

    double sq_sum = std::accumulate(vals.begin(), vals.end(), 0.0,
        [mean](double acc, double v) { return acc + (v - mean) * (v - mean); });
    double stddev = (n > 1) ? std::sqrt(sq_sum / (n - 1.0)) : 0.0;

    std::vector<AnomalyPoint> out;
    out.reserve(points.size());
    for (size_t i = 0; i < points.size(); ++i) {
        AnomalyPoint ap;
        ap.point = points[i];
        if (stddev > 0.0) {
            ap.z_score   = (vals[i] - mean) / stddev;
            ap.is_anomaly = std::abs(ap.z_score) > z_threshold;
        }
        out.push_back(ap);
    }
    return out;
}

}  // namespace zion
