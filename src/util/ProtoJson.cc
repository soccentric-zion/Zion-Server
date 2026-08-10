#include "util/ProtoJson.h"

#include <google/protobuf/util/json_util.h>
#include <spdlog/spdlog.h>

namespace zion::util {

std::string proto_to_json(const google::protobuf::Message& msg) {
    google::protobuf::util::JsonPrintOptions opts;
    opts.add_whitespace                = false;
    opts.always_print_primitive_fields = true;
    opts.preserve_proto_field_names    = true;

    std::string out;
    auto status = google::protobuf::util::MessageToJsonString(msg, &out, opts);
    if (!status.ok()) {
        spdlog::error("proto_to_json failed: {}", status.message().as_string());
        return "";
    }
    return out;
}

bool json_to_proto(const std::string& json, google::protobuf::Message& msg) {
    google::protobuf::util::JsonParseOptions opts;
    opts.ignore_unknown_fields = false;

    auto status = google::protobuf::util::JsonStringToMessage(json, &msg, opts);
    if (!status.ok()) {
        spdlog::error("json_to_proto failed: {}", status.message().as_string());
        return false;
    }
    return true;
}

}  // namespace zion::util
