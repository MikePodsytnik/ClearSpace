#pragma once

#include <utility>
#include <variant>

namespace clearspace::transport {

template <class T, class E>
class Expected {
 public:
  Expected(const T& v) : value_(v) {}
  Expected(T&& v) : value_(std::move(v)) {}
  Expected(const E& e) : value_(e) {}
  Expected(E&& e) : value_(std::move(e)) {}

  bool has_value() const noexcept {
    return std::holds_alternative<T>(value_);
  }

  explicit operator bool() const noexcept {
    return has_value();
  }

  T& value() {
    return std::get<T>(value_);
  }

  const T& value() const {
    return std::get<T>(value_);
  }

  E& error() {
    return std::get<E>(value_);
  }

  const E& error() const {
    return std::get<E>(value_);
  }

 private:
  std::variant<T, E> value_;
};

}  // namespace clearspace::transport
