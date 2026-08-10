#pragma once
// services/ThresholdService.h — evaluate metric thresholds per (module_id, test_id, metric_key).
//
// Thresholds are stored in the `thresholds` table.  When a result is ingested, the caller
// may invoke evaluate() to check all thresholds for the result's module/test and emit
// notifications for any crossings.

#include "repositories/ResultRepository.h"
#include "repositories/NotificationRepository.h"

#include <json/json.h>
#include <optional>
#include <stdexcept>
#include <string>
#include <vector>

namespace zion {

// A single threshold rule.
struct ThresholdRule {
    std::string id;
    std::string module_id;
    std::string test_id;
    std::string metric_key;
    std::string op;          // GT | LT | GE | LE | EQ | NE
    Json::Value value_json;  // { "type": "T", "value": V }
};

// Thrown when a threshold evaluation encounters a type mismatch.
struct ThresholdTypeError : std::runtime_error {
    explicit ThresholdTypeError(const std::string& msg)
        : std::runtime_error("threshold type error: " + msg) {}
};

class ThresholdService {
public:
    // Load all thresholds for (module_id, test_id) and evaluate them against the
    // result's metrics map (parsed from result.metrics_json).
    // Emits a "result.threshold_crossed" notification for each crossing.
    // Returns the number of thresholds crossed.
    int evaluate(const Result& result);

    // CRUD for threshold rules.
    ThresholdRule create_rule(const std::string& module_id,
                               const std::string& test_id,
                               const std::string& metric_key,
                               const std::string& op,
                               const Json::Value& value_json);

    std::vector<ThresholdRule> list_rules(const std::string& module_id,
                                          const std::string& test_id);

    void delete_rule(const std::string& id);

protected:
    // Returns true if lhs <op> rhs according to `op`.
    // Throws ThresholdTypeError if types are incompatible.
    static bool compare(const Json::Value& lhs, const std::string& op,
                        const Json::Value& rhs);

private:
    NotificationRepository notif_repo_;

    // Extract the typed value object from the metrics JSON for a given key.
    // Returns Json::nullValue if the key is absent.
    static Json::Value extract_metric(const Json::Value& metrics_root,
                                       const std::string& key);
};

}  // namespace zion
