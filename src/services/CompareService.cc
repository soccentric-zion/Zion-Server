#include "services/CompareService.h"
#include "util/ResultConvert.h"

#include <cmath>
#include <json/json.h>
#include <sstream>

namespace zion {

// static
std::string CompareService::classify(const CompareEntry& raw,
                                      double drift_pct_threshold) {
    // appeared / disappeared are determined by the repo query (FULL OUTER JOIN).
    if (!raw.run_a_status.has_value()) return "appeared";
    if (!raw.run_b_status.has_value()) return "disappeared";

    int16_t sa = *raw.run_a_status;
    int16_t sb = *raw.run_b_status;

    if (sb > sa) return "regressed";
    if (sb < sa) return "improved";

    // Same status — check severity and metric drift.
    if (raw.run_a_severity != raw.run_b_severity) return "drifted";

    // Parse both metrics JSONs and compare numeric values.
    Json::Value ma = util::parse_typed_map_json(raw.run_a_metrics_json);
    Json::Value mb = util::parse_typed_map_json(raw.run_b_metrics_json);

    for (const auto& key : ma.getMemberNames()) {
        if (!mb.isMember(key)) {
            // Key disappeared — counts as drift.
            return "drifted";
        }
        const Json::Value& va = ma[key];
        const Json::Value& vb = mb[key];
        std::string ta = va.get("type", "").asString();
        std::string tb = vb.get("type", "").asString();
        if (ta != tb) return "drifted";
        if (ta == "double" || ta == "int") {
            double da = va["value"].asDouble();
            double db = vb["value"].asDouble();
            if (std::abs(da) > 1e-15) {
                double pct = std::abs((db - da) / da) * 100.0;
                if (pct > drift_pct_threshold) return "drifted";
            }
        }
    }
    return "unchanged";
}

std::vector<CompareEntry> CompareService::compare(const std::string& run_a,
                                                   const std::string& run_b,
                                                   double drift_pct_threshold) {
    auto raw = result_repo_.find_for_compare(run_a, run_b);
    for (auto& entry : raw) {
        entry.classification = classify(entry, drift_pct_threshold);
    }
    return raw;
}

}  // namespace zion
