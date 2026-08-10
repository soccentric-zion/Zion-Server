#include "services/ThresholdService.h"
#include "util/ResultConvert.h"
#include "db/DbClient.h"

#include <spdlog/spdlog.h>
#include <json/json.h>
#include <sstream>
#include <stdexcept>

namespace zion {

// ── Threshold evaluation ─────────────────────────────────────────────────────

// static
Json::Value ThresholdService::extract_metric(const Json::Value& root,
                                               const std::string& key) {
    if (!root.isMember(key)) return Json::nullValue;
    return root[key];
}

// static
bool ThresholdService::compare(const Json::Value& lhs, const std::string& op,
                                const Json::Value& rhs) {
    if (lhs.isNull() || rhs.isNull())
        return false;

    // Extract the typed value objects.
    const std::string ltype = lhs.get("type", "").asString();
    const std::string rtype = rhs.get("type", "").asString();

    if (op == "EQ" || op == "NE") {
        // EQ/NE: both sides must be the same type.
        if (ltype != rtype)
            throw ThresholdTypeError("EQ/NE requires matching types: " + ltype + " vs " + rtype);

        bool equal;
        if (ltype == "bool")   equal = (lhs["value"].asBool()   == rhs["value"].asBool());
        else if (ltype == "int")    equal = (lhs["value"].asInt64()  == rhs["value"].asInt64());
        else if (ltype == "double") equal = (lhs["value"].asDouble() == rhs["value"].asDouble());
        else                        equal = (lhs["value"].asString() == rhs["value"].asString());

        return (op == "EQ") ? equal : !equal;
    }

    // Numeric ops: both sides must be numeric.
    bool lnum = (ltype == "double" || ltype == "int");
    bool rnum = (rtype == "double" || rtype == "int");
    if (!lnum || !rnum)
        throw ThresholdTypeError("Numeric op " + op + " requires numeric types, got " + ltype + "/" + rtype);

    double lv = lhs["value"].asDouble();
    double rv = rhs["value"].asDouble();

    if (op == "GT") return lv >  rv;
    if (op == "LT") return lv <  rv;
    if (op == "GE") return lv >= rv;
    if (op == "LE") return lv <= rv;

    throw std::runtime_error("Unknown threshold op: " + op);
}

int ThresholdService::evaluate(const Result& result) {
    // Load thresholds for this module/test from the DB.
    auto db = db::get_db_client();
    auto rows = db->execSqlSync(
        "SELECT id, module_id, test_id, metric_key, op, value_json::TEXT"
        " FROM thresholds WHERE module_id = $1 AND test_id = $2",
        result.module_id.c_str(), result.test_id.c_str());

    if (rows.empty()) return 0;

    // Parse the result's metrics.
    Json::Value metrics_root = util::parse_typed_map_json(result.metrics_json);

    int crossings = 0;
    for (const auto& r : rows) {
        ThresholdRule rule;
        rule.id         = r["id"].as<std::string>();
        rule.module_id  = r["module_id"].as<std::string>();
        rule.test_id    = r["test_id"].as<std::string>();
        rule.metric_key = r["metric_key"].as<std::string>();
        rule.op         = r["op"].as<std::string>();

        Json::CharReaderBuilder rb;
        std::string vj = r["value_json"].as<std::string>();
        std::istringstream ss(vj);
        std::string errs;
        Json::parseFromStream(rb, ss, &rule.value_json, &errs);

        Json::Value lhs = extract_metric(metrics_root, rule.metric_key);
        if (lhs.isNull()) continue;

        bool crossed = false;
        try {
            crossed = compare(lhs, rule.op, rule.value_json);
        } catch (const ThresholdTypeError& e) {
            spdlog::warn("threshold: type error for rule {} key={}: {}",
                         rule.id, rule.metric_key, e.what());
            continue;
        }

        if (crossed) {
            ++crossings;
            std::ostringstream payload;
            payload << R"({"run_id":")"    << result.run_id    << "\","
                    << R"("module_id":")"  << result.module_id << "\","
                    << R"("test_id":")"    << result.test_id   << "\","
                    << R"("metric_key":")" << rule.metric_key  << "\","
                    << R"("op":")"         << rule.op          << "\","
                    << R"("result_id":")"  << result.id        << "\"}";
            notif_repo_.create("result.threshold_crossed", payload.str());
            spdlog::info("threshold: crossed — run_id={} module_id={} test_id={} key={} op={}",
                         result.run_id, result.module_id, result.test_id,
                         rule.metric_key, rule.op);
        }
    }
    return crossings;
}

// ── CRUD ─────────────────────────────────────────────────────────────────────

ThresholdRule ThresholdService::create_rule(const std::string& module_id,
                                             const std::string& test_id,
                                             const std::string& metric_key,
                                             const std::string& op,
                                             const Json::Value& value_json) {
    Json::FastWriter fw;
    std::string vj = fw.write(value_json);
    if (!vj.empty() && vj.back() == '\n') vj.pop_back();

    auto db = db::get_db_client();
    auto rows = db->execSqlSync(
        "INSERT INTO thresholds (module_id, test_id, metric_key, op, value_json)"
        " VALUES ($1, $2, $3, $4, $5::JSONB)"
        " RETURNING id, module_id, test_id, metric_key, op, value_json::TEXT",
        module_id.c_str(), test_id.c_str(), metric_key.c_str(),
        op.c_str(), vj.c_str());

    ThresholdRule rule;
    rule.id         = rows[0]["id"].as<std::string>();
    rule.module_id  = rows[0]["module_id"].as<std::string>();
    rule.test_id    = rows[0]["test_id"].as<std::string>();
    rule.metric_key = rows[0]["metric_key"].as<std::string>();
    rule.op         = rows[0]["op"].as<std::string>();
    Json::CharReaderBuilder rb;
    std::string s = rows[0]["value_json"].as<std::string>();
    std::istringstream ss(s);
    std::string errs;
    Json::parseFromStream(rb, ss, &rule.value_json, &errs);
    return rule;
}

std::vector<ThresholdRule> ThresholdService::list_rules(const std::string& module_id,
                                                          const std::string& test_id) {
    auto db = db::get_db_client();
    auto rows = db->execSqlSync(
        "SELECT id, module_id, test_id, metric_key, op, value_json::TEXT"
        " FROM thresholds WHERE module_id = $1 AND test_id = $2"
        " ORDER BY metric_key, op",
        module_id.c_str(), test_id.c_str());

    std::vector<ThresholdRule> out;
    for (const auto& r : rows) {
        ThresholdRule rule;
        rule.id         = r["id"].as<std::string>();
        rule.module_id  = r["module_id"].as<std::string>();
        rule.test_id    = r["test_id"].as<std::string>();
        rule.metric_key = r["metric_key"].as<std::string>();
        rule.op         = r["op"].as<std::string>();
        Json::CharReaderBuilder rb;
        std::string s = r["value_json"].as<std::string>();
        std::istringstream ss(s);
        std::string errs;
        Json::parseFromStream(rb, ss, &rule.value_json, &errs);
        out.push_back(rule);
    }
    return out;
}

void ThresholdService::delete_rule(const std::string& id) {
    auto db = db::get_db_client();
    db->execSqlSync("DELETE FROM thresholds WHERE id = $1::UUID", id.c_str());
}

}  // namespace zion
