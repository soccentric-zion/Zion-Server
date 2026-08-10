#pragma once
// util/Errors.h — common error types and gRPC/HTTP status helpers.
#include <stdexcept>
#include <string>

namespace zion {

// Base application error.
struct AppError : std::runtime_error {
    using std::runtime_error::runtime_error;
};

struct NotFoundError : AppError {
    explicit NotFoundError(const std::string& msg) : AppError("not found: " + msg) {}
};

struct ValidationError : AppError {
    explicit ValidationError(const std::string& msg) : AppError("validation: " + msg) {}
};

struct ConflictError : AppError {
    explicit ConflictError(const std::string& msg) : AppError("conflict: " + msg) {}
};

struct AuthError : AppError {
    explicit AuthError(const std::string& msg) : AppError("auth: " + msg) {}
};

struct ForbiddenError : AppError {
    explicit ForbiddenError(const std::string& msg) : AppError("forbidden: " + msg) {}
};

struct ServiceUnavailableError : AppError {
    explicit ServiceUnavailableError(const std::string& msg)
        : AppError("service unavailable: " + msg) {}
};

}  // namespace zion
