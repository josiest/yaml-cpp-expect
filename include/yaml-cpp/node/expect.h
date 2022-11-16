#pragma once // gcc >= 12 in order for this header to work

#include "yaml-cpp/node/node.h"
#include "yaml-cpp/node/conversion.h" // ConvertStreamTo
#include "yaml-cpp/exceptions.h"
#include "yaml-cpp/null.h"
#include <concepts>

namespace YAML {
template<typename T>
struct expect;

template<>
struct expect<std::string> {
  Expected<std::string> operator()(const Node& node) const noexcept {
    if (!node.IsScalar()) {
      return Unexpected(node, ErrorMsg::NOT_A_STRING);
    }
    return node.Scalar();
  }
};

template<>
struct expect<_Null> {
  Expected<void> operator()(const Node& node) const noexcept {
    if (!node.IsNull()) {
      return Unexpected(node.Mark(), ErrorMsg::NOT_NULL);
    }
    return {};
  }
};

template<std::integral T>
requires (!std::same_as<T, bool>)
struct expect<T> {
  Expected<T> operator()(const Node& node) const noexcept {
    if (node.Type() != NodeType::Scalar) {
      return Unexpected(node, ErrorMsg::NOT_AN_INTEGER);
    }
    std::stringstream stream(node.Scalar());
    if (std::is_unsigned_v<T> && (stream.peek() == '-')) {
      return Unexpected(node, ErrorMsg::NOT_NON_NEGATIVE);
    }
    T rhs;
    if (conversion::ConvertStreamTo(stream, rhs)) {
      return rhs;
    }
    return Unexpected(node, ErrorMsg::BAD_CONVERSION);
  }
};

template<std::floating_point T>
struct expect<T> {
  Expected<void> operator()(const Node& node) const noexcept {
    if (node.Type() != NodeType::Scalar) {
      return Unexpected(node, ErrorMsg::NOT_A_FLOAT);
    }
    const std::string& input = node.Scalar();
    std::stringstream stream(input);
    stream.unsetf(std::ios::dec);
    T rhs;
    if (conversion::ConvertStreamTo(stream, rhs)) {
      return rhs;
    }
    if (std::numeric_limits<T>::has_infinity) {
      if (conversion::IsInfinity(input)) {
        rhs = std::numeric_limits<T>::infinity();
        return rhs;
      } else if (conversion::IsNegativeInfinity(input)) {
        rhs = -std::numeric_limits<T>::infinity();
        return rhs;
      }
    }
    if (std::numeric_limits<T>::has_quiet_NaN) {
      if (conversion::IsNaN(input)) {
        rhs = std::numeric_limits<T>::quiet_NaN();
        return rhs;
      }
    }
    return Unexpected(node, ErrorMsg::BAD_CONVERSION);
  }
};

template<>
struct expect<bool> {
  YAML_CPP_API Expected<bool> operator()(const Node& node) const noexcept;
};
}
