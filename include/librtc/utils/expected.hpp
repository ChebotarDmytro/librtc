#pragma once

#include <system_error>
#include <type_traits>
#include <utility>
#include <variant>

namespace librtc {

/**
 * Result<T, E> - A C++20 implementation similar to std::expected (C++23)
 */

struct ErrorTag {};
inline constexpr ErrorTag Error{};

template <typename E>
struct ErrorValue {
  E error;

  explicit ErrorValue(E e) : error(std::move(e)) {}

  // Converting constructor for error code enums
  template <typename U, typename = std::enable_if_t<std::is_error_code_enum_v<U> &&
                                                    std::is_same_v<E, std::error_code>>>
  ErrorValue(U e) : error(make_error_code(e)) {}
};

template <typename E>
ErrorValue(E) -> ErrorValue<E>;

template <typename T, typename E = std::error_code>
class [[nodiscard]] Result;

// Result for void
template <typename E>
class [[nodiscard]] Result<void, E> {
 public:
  Result() : storage_(SuccessTag{}) {}

  Result(ErrorValue<E> err) : storage_(std::move(err.error)) {}

  // Converting constructor for error code enums
  template <typename OtherE, typename = std::enable_if_t<std::is_error_code_enum_v<OtherE> &&
                                                         std::is_same_v<E, std::error_code>>>
  Result(ErrorValue<OtherE> err) : storage_(make_error_code(err.error)) {}

  // Converting constructor from Result with different error type
  template <typename OtherE, typename = std::enable_if_t<std::is_error_code_enum_v<OtherE> &&
                                                         std::is_same_v<E, std::error_code>>>
  Result(const Result<void, OtherE>& other) {
    if (other.has_value()) {
      storage_ = SuccessTag{};
    } else {
      storage_ = make_error_code(other.error());
    }
  }

  explicit operator bool() const noexcept {
    return has_value();
  }

  [[nodiscard]] bool has_value() const noexcept {
    return std::holds_alternative<SuccessTag>(storage_);
  }

  [[nodiscard]] bool has_error() const noexcept {
    return !has_value();
  }

  [[nodiscard]] const E& error() const& {
    return std::get<E>(storage_);
  }

  [[nodiscard]] E& error() & {
    return std::get<E>(storage_);
  }

  [[nodiscard]] E&& error() && {
    return std::get<E>(std::move(storage_));
  }

  template <typename F>
  auto and_then(F&& f) const& -> decltype(f()) {
    if (has_value()) {
      return f();
    }
    return ErrorValue{error()};
  }

  template <typename F>
  auto or_else(F&& f) const& -> Result {
    if (has_error()) {
      return f(error());
    }
    return {};
  }

 private:
  struct SuccessTag {};
  std::variant<SuccessTag, E> storage_;
};

// Result for non-void types
template <typename T, typename E>
class [[nodiscard]] Result {
 public:
  Result(T value) : storage_(std::move(value)) {}

  Result(ErrorValue<E> err) : storage_(std::move(err.error)) {}

  // Converting constructor for error code enums
  template <typename OtherE, typename = std::enable_if_t<std::is_error_code_enum_v<OtherE> &&
                                                         std::is_same_v<E, std::error_code>>>
  Result(ErrorValue<OtherE> err) : storage_(make_error_code(err.error)) {}

  // Converting constructor from Result with different error type (for error code enums)
  template <typename OtherE, typename = std::enable_if_t<std::is_error_code_enum_v<OtherE> &&
                                                         std::is_same_v<E, std::error_code>>>
  Result(const Result<T, OtherE>& other) {
    if (other.has_value()) {
      storage_ = other.value();
    } else {
      storage_ = make_error_code(other.error());
    }
  }

  explicit operator bool() const noexcept {
    return has_value();
  }

  [[nodiscard]] bool has_value() const noexcept {
    return std::holds_alternative<T>(storage_);
  }

  [[nodiscard]] bool has_error() const noexcept {
    return !has_value();
  }

  [[nodiscard]] const T& value() const& {
    return std::get<T>(storage_);
  }

  [[nodiscard]] T& value() & {
    return std::get<T>(storage_);
  }

  [[nodiscard]] T&& value() && {
    return std::get<T>(std::move(storage_));
  }

  [[nodiscard]] const E& error() const& {
    return std::get<E>(storage_);
  }

  [[nodiscard]] E& error() & {
    return std::get<E>(storage_);
  }

  [[nodiscard]] E&& error() && {
    return std::get<E>(std::move(storage_));
  }

  template <typename U>
  [[nodiscard]] T value_or(U&& default_value) const& {
    if (has_value()) {
      return value();
    }
    return static_cast<T>(std::forward<U>(default_value));
  }

  template <typename F>
  auto and_then(F&& f) const& -> decltype(f(value())) {
    if (has_value()) {
      return f(value());
    }
    return ErrorValue{error()};
  }

  template <typename F>
  auto or_else(F&& f) const& -> Result {
    if (has_error()) {
      return f(error());
    }
    return value();
  }

  template <typename F>
  auto transform(F&& f) const& -> Result<decltype(f(value())), E> {
    if (has_value()) {
      return f(value());
    }
    return ErrorValue{error()};
  }

 private:
  std::variant<T, E> storage_;
};

// Generic success helper that can convert to any Result<void, E>
struct SuccessType {
  template <typename E>
  operator Result<void, E>() const {
    return Result<void, E>();
  }
};

inline constexpr SuccessType Success() {
  return {};
}

template <typename T, typename E = std::error_code>
inline Result<T, E> Success(T value) {
  return Result<T, E>(std::move(value));
}

template <typename E>
inline auto Err(E error) {
  return ErrorValue{std::move(error)};
}

template <typename T>
using Expected = Result<T, std::error_code>;

}  // namespace librtc
