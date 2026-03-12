#pragma once

#include <utility>
#include <variant>

namespace clearspace::core {

template <class T, class E>
class Expected {
 public:
  Expected(const T& v) : v_(v) {}
  Expected(T&& v) : v_(std::move(v)) {}
  Expected(const E& e) : v_(e) {}
  Expected(E&& e) : v_(std::move(e)) {}

  bool has_value() const noexcept { return std::holds_alternative<T>(v_); }
  explicit operator bool() const noexcept { return has_value(); }

  T& value() { return std::get<T>(v_); }
  const T& value() const { return std::get<T>(v_); }

  E& error() { return std::get<E>(v_); }
  const E& error() const { return std::get<E>(v_); }

 private:
  std::variant<T, E> v_;
};

template <class E>
class Expected<void, E> {
 public:
  Expected() : v_(std::monostate{}) {}
  Expected(const E& e) : v_(e) {}
  Expected(E&& e) : v_(std::move(e)) {}

  bool has_value() const noexcept { return std::holds_alternative<std::monostate>(v_); }
  explicit operator bool() const noexcept { return has_value(); }

  void value() const {}

  E& error() { return std::get<E>(v_); }
  const E& error() const { return std::get<E>(v_); }

 private:
  std::variant<std::monostate, E> v_;
};

}
