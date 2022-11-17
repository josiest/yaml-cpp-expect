#pragma once // gcc >= 12 in order for this header to work

#include "yaml-cpp/node/node.h"
#include "yaml-cpp/node/conversion.h" // ConvertStreamTo
#include "yaml-cpp/exceptions.h"
#include "yaml-cpp/null.h"

#include <concepts>
#include <type_traits>
#include <iterator>
#include <utility>

namespace YAML {
template<typename T>
struct expect;

template<>
struct expect<Node> {
  Expected<Node> operator()(const Node& node) const noexcept {
    return node;
  }
};

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

template<typename T>
using expected_error_t = typename T::error_type;

template<typename T>
using expected_value_t = typename T::value_type;

template<typename R, typename V, typename E>
concept ExpectedLike = requires(const R& result) {
  { !result } -> std::convertible_to<bool>;
  { *result } -> std::convertible_to<V>;
  { result.error() } -> std::convertible_to<E>;
};

template<typename T, typename N = Node>
concept Expectable = requires(const expect<T>& expect_fn, const N& node) {
  { expect_fn(node) } -> ExpectedLike<T, Exception>;
  { expect_fn(node).error() } -> std::convertible_to<Exception>;
  { *expect_fn(node) } -> std::convertible_to<T>;
};

template<typename T>
using tuple_first_t = typename std::tuple_element_t<0, std::remove_const_t<T>>;
template<typename T>
using tuple_second_t = typename std::tuple_element_t<1, std::remove_const_t<T>>;

// from https://en.cppreference.com/w/cpp/ranges/subrange/operator_PairLike
template<class T>
concept PairLike = !std::is_reference_v<T> &&
requires(T t) {
  typename std::tuple_size<T>::type;   // ensures std::tuple_size<T> is complete
  requires std::derived_from<std::tuple_size<T>,
                             std::integral_constant<std::size_t, 2>>;
  typename tuple_first_t<T>;
  typename tuple_second_t<T>;
  { std::get<0>(t) } -> std::convertible_to<const std::tuple_element_t<0, T>&>;
  { std::get<1>(t) } -> std::convertible_to<const std::tuple_element_t<1, T>&>;
};

template<PairLike Pair>
requires Expectable<tuple_first_t<Pair>> &&
         Expectable<tuple_second_t<Pair>>
struct expect<Pair> {
  Expected<Pair> operator()(const std::pair<Node, Node>& node) const noexcept
  {
    auto const&[first_node, second_node] = node;
    const auto first = first_node.expect<tuple_first_t<Pair>>();
    if (!first) {
      return std::unexpected(first.error());
    }
    const auto second = second_node.expect<tuple_second_t<Pair>>();
    if (!second) {
      return std::unexpected(second.error());
    }
    return Pair{*first, *second};
  }
};

template<typename T>
concept PairExpectable = PairLike<T> and Expectable<T, std::pair<Node, Node>>;

template<typename T>
concept ContainerExpectable = Expectable<T> or PairExpectable<T>;

template <ContainerExpectable T,
          std::weakly_incrementable ValueOutput,
          std::weakly_incrementable ErrorOutput>

requires std::indirectly_writable<ValueOutput, T> &&
         std::indirectly_writable<ErrorOutput, Exception>

std::pair<ValueOutput, ErrorOutput>
partition_expect(const Node& node, ValueOutput values,
                                   ErrorOutput errors) noexcept {
  namespace views = std::views;
  if (!node) {
    *errors++ = Exception(node.Mark(), ErrorMsg::INVALID_NODE);
    return std::make_pair(values, errors);
  }
  if (!node.IsMap() && !node.IsSequence()) {
    *errors++ = Exception(node.Mark(), ErrorMsg::NOT_A_CONTAINER);
    return std::make_pair(values, errors);
  }
  const expect<T> read_value;
  for (auto && result : node | views::transform(read_value)) {
    if (!result) {
      *errors++ = std::move(result).error();
    }
    else {
      *values++ = *std::move(result);
    }
  }
  return std::make_pair(values, errors);
}

}