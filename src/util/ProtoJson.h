#pragma once
// util/ProtoJson.h — helpers for converting proto messages to/from JSON,
// using the protobuf util library with proper Any expansion.

#include <google/protobuf/message.h>

#include <string>

namespace zion::util {

// Serialize a proto message to a JSON string.
// Returns an empty string on failure (logs the error).
std::string proto_to_json(const google::protobuf::Message& msg);

// Deserialise a JSON string into a proto message.
// Returns false and logs on failure.
bool json_to_proto(const std::string& json, google::protobuf::Message& msg);

}  // namespace zion::util
