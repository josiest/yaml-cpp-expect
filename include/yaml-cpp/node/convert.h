#ifndef NODE_CONVERT_H_62B23520_7C8E_11DE_8A39_0800200C9A66
#define NODE_CONVERT_H_62B23520_7C8E_11DE_8A39_0800200C9A66

#if defined(_MSC_VER) ||                                            \
    (defined(__GNUC__) && (__GNUC__ == 3 && __GNUC_MINOR__ >= 4) || \
     (__GNUC__ >= 4))  // GCC supports "pragma once" correctly since 3.4
#pragma once
#endif

#include <array>
#include <cmath>
#include <limits>
#include <list>
#include <map>
#include <unordered_map>
#include <sstream>
#include <type_traits>
#include <valarray>
#include <vector>

#if __cplusplus > 202002L
#include <expected>
#include <concepts>
#include "yaml-cpp/exceptions.h"
#endif

#include "yaml-cpp/binary.h"
#include "yaml-cpp/node/impl.h"
#include "yaml-cpp/node/iterator.h"
#include "yaml-cpp/node/node.h"
#include "yaml-cpp/node/type.h"
#include "yaml-cpp/null.h"


namespace YAML {
class Binary;
struct _Null;
template <typename T>
struct convert;
}  // namespace YAML

namespace YAML {
namespace conversion {
inline bool IsInfinity(const std::string& input) {
  return input == ".inf" || input == ".Inf" || input == ".INF" ||
         input == "+.inf" || input == "+.Inf" || input == "+.INF";
}

inline bool IsNegativeInfinity(const std::string& input) {
  return input == "-.inf" || input == "-.Inf" || input == "-.INF";
}

inline bool IsNaN(const std::string& input) {
  return input == ".nan" || input == ".NaN" || input == ".NAN";
}
}

// Node
template <>
struct convert<Node> {
  static Node encode(const Node& rhs) { return rhs; }

  static bool decode(const Node& node, Node& rhs) {
    rhs.reset(node);
    return true;
  }

#if __cplusplus > 202002L
  static Expected<void> expect(const Node& node, Node& rhs) noexcept {
    if (!node.IsDefined() || !rhs.IsDefined()) {
      return std::unexpected(Exception(rhs.Mark(), ErrorMsg::INVALID_NODE));
    }
    return {};
  }
#endif
};

// std::string
template <>
struct convert<std::string> {
  static Node encode(const std::string& rhs) { return Node(rhs); }

  static bool decode(const Node& node, std::string& rhs) {
    if (!node.IsScalar())
      return false;
    rhs = node.Scalar();
    return true;
  }
#if __cplusplus > 202002L
  static Expected<void> expect(const Node& node, std::string& rhs) noexcept {
    if (!node.IsDefined())
      return std::unexpected(Exception(node.Mark(), ErrorMsg::INVALID_NODE));
    if (!node.IsScalar())
      return std::unexpected(Exception(node.Mark(), ErrorMsg::NOT_A_STRING));
    rhs = node.Scalar();
    return {};
  }
#endif
};

// C-strings can only be encoded
template <>
struct convert<const char*> {
  static Node encode(const char* rhs) { return Node(rhs); }
};

template <>
struct convert<char*> {
  static Node encode(const char* rhs) { return Node(rhs); }
};

template <std::size_t N>
struct convert<char[N]> {
  static Node encode(const char* rhs) { return Node(rhs); }
};

template <>
struct convert<_Null> {
  static Node encode(const _Null& /* rhs */) { return Node(); }

  static bool decode(const Node& node, _Null& /* rhs */) {
    return node.IsNull();
  }
#if __cplusplus > 202002L
  static Expected<void> expect(const Node& node, _Null& /* rhs */) {
    if (!node.IsNull())
      return std::unexpected(Exception(node.Mark(), ErrorMsg::INVALID_NODE));
    return {};
  }
#endif
};

#if __cplusplus >= 202002L
template <typename T>
concept OutputStreamable = requires(const T& t, std::stringstream & stream) {
    { stream << t };
};
template <typename T>
concept InputStreamable = requires(T& t, std::stringstream & stream) {
    { stream >> t };
};
#endif

namespace conversion {
#if __cplusplus < 202002L
template <typename T>
typename std::enable_if< std::is_floating_point<T>::value, void>::type
#else
template <OutputStreamable T>
requires std::floating_point<T>
void
#endif
inner_encode(const T& rhs, std::stringstream& stream){
  if (std::isnan(rhs)) {
    stream << ".nan";
  } else if (std::isinf(rhs)) {
    if (std::signbit(rhs)) {
      stream << "-.inf";
    } else {
      stream << ".inf";
    }
  } else {
    stream << rhs;
  }
}

#if __cplusplus < 202002L
template <typename T>
typename std::enable_if<!std::is_floating_point<T>::value, void>::type
#else
// not ambiguous since C++ will prefer "more specialized" types
template <OutputStreamable T>
void
#endif
inner_encode(const T& rhs, std::stringstream& stream){
  stream << rhs;
}

#if __cplusplus < 202002L
template <typename T>
typename std::enable_if<(std::is_same<T, unsigned char>::value ||
                         std::is_same<T, signed char>::value), bool>::type
#else
template <typename T>
requires (std::same_as<T, signed char> || std::same_as<T, unsigned char>)
bool
#endif
ConvertStreamTo(std::stringstream& stream, T& rhs) {
  int num;
  if ((stream >> std::noskipws >> num) && (stream >> std::ws).eof()) {
    if (num >= (std::numeric_limits<T>::min)() &&
        num <= (std::numeric_limits<T>::max)()) {
      rhs = static_cast<T>(num);
      return true;
    }
  }
  return false;
}

#if __cplusplus < 202002L
template <typename T>
typename std::enable_if<!(std::is_same<T, unsigned char>::value ||
                          std::is_same<T, signed char>::value), bool>::type
#else
template <InputStreamable T>
bool
#endif
ConvertStreamTo(std::stringstream& stream, T& rhs) {
  if ((stream >> std::noskipws >> rhs) && (stream >> std::ws).eof()) {
    return true;
  }
  return false;
}
}

#if __cplusplus <= 202002L
#define YAML_DEFINE_CONVERT_STREAMABLE(type, negative_op)                  \
  template <>                                                              \
  struct convert<type> {                                                   \
                                                                           \
    static Node encode(const type& rhs) {                                  \
      std::stringstream stream;                                            \
      stream.precision(std::numeric_limits<type>::max_digits10);           \
      conversion::inner_encode(rhs, stream);                               \
      return Node(stream.str());                                           \
    }                                                                      \
                                                                           \
    static bool decode(const Node& node, type& rhs) {                      \
      if (node.Type() != NodeType::Scalar) {                               \
        return false;                                                      \
      }                                                                    \
      const std::string& input = node.Scalar();                            \
      std::stringstream stream(input);                                     \
      stream.unsetf(std::ios::dec);                                        \
      if ((stream.peek() == '-') && std::is_unsigned<type>::value) {       \
        return false;                                                      \
      }                                                                    \
      if (conversion::ConvertStreamTo(stream, rhs)) {                      \
        return true;                                                       \
      }                                                                    \
      if (std::numeric_limits<type>::has_infinity) {                       \
        if (conversion::IsInfinity(input)) {                               \
          rhs = std::numeric_limits<type>::infinity();                     \
          return true;                                                     \
        } else if (conversion::IsNegativeInfinity(input)) {                \
          rhs = negative_op std::numeric_limits<type>::infinity();         \
          return true;                                                     \
        }                                                                  \
      }                                                                    \
                                                                           \
      if (std::numeric_limits<type>::has_quiet_NaN) {                      \
        if (conversion::IsNaN(input)) {                                    \
          rhs = std::numeric_limits<type>::quiet_NaN();                    \
          return true;                                                     \
        }                                                                  \
      }                                                                    \
                                                                           \
      return false;                                                        \
    }                                                                      \
  }

#define YAML_DEFINE_CONVERT_STREAMABLE_SIGNED(type) \
  YAML_DEFINE_CONVERT_STREAMABLE(type, -)

#define YAML_DEFINE_CONVERT_STREAMABLE_UNSIGNED(type) \
  YAML_DEFINE_CONVERT_STREAMABLE(type, +)

YAML_DEFINE_CONVERT_STREAMABLE_SIGNED(int);
YAML_DEFINE_CONVERT_STREAMABLE_SIGNED(short);
YAML_DEFINE_CONVERT_STREAMABLE_SIGNED(long);
YAML_DEFINE_CONVERT_STREAMABLE_SIGNED(long long);
YAML_DEFINE_CONVERT_STREAMABLE_UNSIGNED(unsigned);
YAML_DEFINE_CONVERT_STREAMABLE_UNSIGNED(unsigned short);
YAML_DEFINE_CONVERT_STREAMABLE_UNSIGNED(unsigned long);
YAML_DEFINE_CONVERT_STREAMABLE_UNSIGNED(unsigned long long);

YAML_DEFINE_CONVERT_STREAMABLE_SIGNED(char);
YAML_DEFINE_CONVERT_STREAMABLE_SIGNED(signed char);
YAML_DEFINE_CONVERT_STREAMABLE_UNSIGNED(unsigned char);

YAML_DEFINE_CONVERT_STREAMABLE_SIGNED(float);
YAML_DEFINE_CONVERT_STREAMABLE_SIGNED(double);
YAML_DEFINE_CONVERT_STREAMABLE_SIGNED(long double);

#undef YAML_DEFINE_CONVERT_STREAMABLE_SIGNED
#undef YAML_DEFINE_CONVERT_STREAMABLE_UNSIGNED
#undef YAML_DEFINE_CONVERT_STREAMABLE

#else // __cplusplus > 202002L
template <std::integral T>
requires (!std::same_as<T, bool>)
struct convert<T> {
  static Node encode(const T& rhs) {
    return Node(std::to_string(rhs));
  }

  static Expected<void> expect(const Node& node, T& rhs) {
    if (!node.IsDefined()) {
      return Unexpected(node, ErrorMsg::INVALID_NODE);
    }
    if (node.Type() != NodeType::Scalar) {
      return Unexpected(node, ErrorMsg::NOT_AN_INTEGER);
    }
    std::stringstream stream(node.Scalar());
    if (std::is_unsigned_v<T> && (stream.peek() == '-')) {
      return Unexpected(node, ErrorMsg::NOT_NON_NEGATIVE);
    }
    if (conversion::ConvertStreamTo(stream, rhs)) {
      return {};
    }
    return Unexpected(node, ErrorMsg::BAD_CONVERSION);
  }

  static bool decode(const Node& node, T& rhs) {
    auto result = convert<T>::expect(node, rhs);
    if (not result) {
      throw result.error();
    }
    return true;
  }
};

template <std::floating_point T>
struct convert<T> {

  static Node encode(const T& rhs) {
    std::stringstream stream;
    stream.precision(std::numeric_limits<T>::max_digits10);
    conversion::inner_encode(rhs, stream);
    return Node(stream.str());
  }

  static Expected<void> expect(const Node& node, T& rhs) {
    if (!node.IsDefined()) {
      return Unexpected(node, ErrorMsg::INVALID_NODE);
    }
    if (node.Type() != NodeType::Scalar) {
      return Unexpected(node, ErrorMsg::NOT_A_FLOAT);
    }
    const std::string& input = node.Scalar();
    std::stringstream stream(input);
    stream.unsetf(std::ios::dec);
    if (conversion::ConvertStreamTo(stream, rhs)) {
      return {};
    }
    if (std::numeric_limits<T>::has_infinity) {
      if (conversion::IsInfinity(input)) {
        rhs = std::numeric_limits<T>::infinity();
        return {};
      } else if (conversion::IsNegativeInfinity(input)) {
        rhs = -std::numeric_limits<T>::infinity();
        return {};
      }
    }

    if (std::numeric_limits<T>::has_quiet_NaN) {
      if (conversion::IsNaN(input)) {
        rhs = std::numeric_limits<T>::quiet_NaN();
        return {};
      }
    }

    return Unexpected(node, ErrorMsg::BAD_CONVERSION);
  }

  static bool decode(const Node& node, T& rhs) {
    auto result = convert<T>::expect(node, rhs);
    if (not result) {
      throw result.error();
    }
    return true;
  }
};
#endif // __cplusplus > 202002L

// bool
template <>
struct convert<bool> {
  static Node encode(bool rhs) { return rhs ? Node("true") : Node("false"); }

  YAML_CPP_API static bool decode(const Node& node, bool& rhs);
#if __cplusplus > 202002L
  YAML_CPP_API static Expected<void> expect(const Node& node, bool& rhs);
#endif
};

// std::map
template <typename K, typename V, typename C, typename A>
struct convert<std::map<K, V, C, A>> {
  static Node encode(const std::map<K, V, C, A>& rhs) {
    Node node(NodeType::Map);
    for (const auto& element : rhs)
      node.force_insert(element.first, element.second);
    return node;
  }

  static bool decode(const Node& node, std::map<K, V, C, A>& rhs) {
    if (!node.IsMap())
      return false;

    rhs.clear();
    for (const auto& element : node)
#if defined(__GNUC__) && __GNUC__ < 4
      // workaround for GCC 3:
      rhs[element.first.template as<K>()] = element.second.template as<V>();
#else
      rhs[element.first.as<K>()] = element.second.as<V>();
#endif
    return true;
  }
#if __cplusplus > 202002L
  static Expected<void> expect(const Node& node, std::map<K, V, C, A>& rhs) {
    if (!node.IsDefined())
      return Unexpected(node, ErrorMsg::INVALID_NODE);
    if (!node.IsMap())
      return Unexpected(node, ErrorMsg::NOT_A_MAP);
    if (!decode(node, rhs))
      return Unexpected(node, ErrorMsg::BAD_CONVERSION);
    return {};
  }
#endif
};

// std::unordered_map
template <typename K, typename V, typename H, typename P, typename A>
struct convert<std::unordered_map<K, V, H, P, A>> {
  static Node encode(const std::unordered_map<K, V, H, P, A>& rhs) {
    Node node(NodeType::Map);
    for (const auto& element : rhs)
      node.force_insert(element.first, element.second);
    return node;
  }

  static bool decode(const Node& node, std::unordered_map<K, V, H, P, A>& rhs) {
    if (!node.IsMap())
      return false;

    rhs.clear();
    for (const auto& element : node)
#if defined(__GNUC__) && __GNUC__ < 4
      // workaround for GCC 3:
      rhs[element.first.template as<K>()] = element.second.template as<V>();
#else
      rhs[element.first.as<K>()] = element.second.as<V>();
#endif
    return true;
  }
#if __cplusplus > 202002L
  static Expected<void> expect(const Node& node,
                               std::unordered_map<K, V, H, P, A>& rhs) {
    if (!node.IsDefined())
      return Unexpected(node, ErrorMsg::INVALID_NODE);
    if (!node.IsMap())
      return Unexpected(node, ErrorMsg::NOT_A_MAP);
    if (!decode(node, rhs))
      return Unexpected(node, ErrorMsg::BAD_CONVERSION);
    return {};
  }
#endif
};

// std::vector
template <typename T, typename A>
struct convert<std::vector<T, A>> {
  static Node encode(const std::vector<T, A>& rhs) {
    Node node(NodeType::Sequence);
    for (const auto& element : rhs)
      node.push_back(element);
    return node;
  }

  static bool decode(const Node& node, std::vector<T, A>& rhs) {
    if (!node.IsSequence())
      return false;

    rhs.clear();
    for (const auto& element : node)
#if defined(__GNUC__) && __GNUC__ < 4
      // workaround for GCC 3:
      rhs.push_back(element.template as<T>());
#else
      rhs.push_back(element.as<T>());
#endif
    return true;
  }
#if __cplusplus > 202002L
  static Expected<void> expect(const Node& node, std::vector<T, A>& rhs) {
    if (!node.IsDefined())
      return Unexpected(node, ErrorMsg::INVALID_NODE);
    if (!node.IsSequence())
      return Unexpected(node, ErrorMsg::NOT_A_SEQUENCE);
    if (!decode(node, rhs))
      return Unexpected(node, ErrorMsg::BAD_CONVERSION);
    return {};
  }
#endif
};

// std::list
template <typename T, typename A>
struct convert<std::list<T,A>> {
  static Node encode(const std::list<T,A>& rhs) {
    Node node(NodeType::Sequence);
    for (const auto& element : rhs)
      node.push_back(element);
    return node;
  }

  static bool decode(const Node& node, std::list<T,A>& rhs) {
    if (!node.IsSequence())
      return false;

    rhs.clear();
    for (const auto& element : node)
#if defined(__GNUC__) && __GNUC__ < 4
      // workaround for GCC 3:
      rhs.push_back(element.template as<T>());
#else
      rhs.push_back(element.as<T>());
#endif
    return true;
  }
#if __cplusplus > 202002L
  static Expected<void> expect(const Node& node, std::list<T, A>& rhs) {
    if (!node.IsDefined())
      return Unexpected(node, ErrorMsg::INVALID_NODE);
    if (!node.IsSequence())
      return Unexpected(node, ErrorMsg::NOT_A_SEQUENCE);
    if (!decode(node, rhs))
      return Unexpected(node, ErrorMsg::BAD_CONVERSION);
    return {};
  }
#endif
};

// std::array
template <typename T, std::size_t N>
struct convert<std::array<T, N>> {
  static Node encode(const std::array<T, N>& rhs) {
    Node node(NodeType::Sequence);
    for (const auto& element : rhs) {
      node.push_back(element);
    }
    return node;
  }

  static bool decode(const Node& node, std::array<T, N>& rhs) {
    if (!isNodeValid(node)) {
      return false;
    }

    for (auto i = 0u; i < node.size(); ++i) {
#if defined(__GNUC__) && __GNUC__ < 4
      // workaround for GCC 3:
      rhs[i] = node[i].template as<T>();
#else
      rhs[i] = node[i].as<T>();
#endif
    }
    return true;
  }
#if __cplusplus > 202002L
  static Expected<void> expect(const Node& node, std::array<T, N>& rhs) {
    if (!node.IsDefined())
      return Unexpected(node, ErrorMsg::INVALID_NODE);
    if (!node.IsSequence())
      return Unexpected(node, ErrorMsg::NOT_A_SEQUENCE);
    if (!decode(node, rhs))
      return Unexpected(node, ErrorMsg::BAD_CONVERSION);
    return {};
  }
#endif

 private:
  static bool isNodeValid(const Node& node) {
    return node.IsSequence() && node.size() == N;
  }
};


// std::valarray
template <typename T>
struct convert<std::valarray<T>> {
  static Node encode(const std::valarray<T>& rhs) {
    Node node(NodeType::Sequence);
    for (const auto& element : rhs) {
      node.push_back(element);
    }
    return node;
  }

  static bool decode(const Node& node, std::valarray<T>& rhs) {
    if (!node.IsSequence()) {
      return false;
    }

    rhs.resize(node.size());
    for (auto i = 0u; i < node.size(); ++i) {
#if defined(__GNUC__) && __GNUC__ < 4
      // workaround for GCC 3:
      rhs[i] = node[i].template as<T>();
#else
      rhs[i] = node[i].as<T>();
#endif
    }
    return true;
  }
#if __cplusplus > 202002L
  static Expected<void> expect(const Node& node, std::valarray<T>& rhs) {
    if (!node.IsDefined())
      return Unexpected(node, ErrorMsg::INVALID_NODE);
    if (!node.IsSequence())
      return Unexpected(node, ErrorMsg::NOT_A_SEQUENCE);
    if (!decode(node, rhs))
      return Unexpected(node, ErrorMsg::BAD_CONVERSION);
    return {};
  }
#endif
};


// std::pair
template <typename T, typename U>
struct convert<std::pair<T, U>> {
  static Node encode(const std::pair<T, U>& rhs) {
    Node node(NodeType::Sequence);
    node.push_back(rhs.first);
    node.push_back(rhs.second);
    return node;
  }

  static bool decode(const Node& node, std::pair<T, U>& rhs) {
    if (!node.IsSequence())
      return false;
    if (node.size() != 2)
      return false;

#if defined(__GNUC__) && __GNUC__ < 4
    // workaround for GCC 3:
    rhs.first = node[0].template as<T>();
#else
    rhs.first = node[0].as<T>();
#endif
#if defined(__GNUC__) && __GNUC__ < 4
    // workaround for GCC 3:
    rhs.second = node[1].template as<U>();
#else
    rhs.second = node[1].as<U>();
#endif
    return true;
  }
#if __cplusplus > 202002L
  static Expected<void> expect(const Node& node, std::pair<T, U>& rhs) {
    if (!node.IsDefined())
      return Unexpected(node, ErrorMsg::INVALID_NODE);
    if (!node.IsSequence() || node.size() != 2)
      return Unexpected(node, ErrorMsg::NOT_A_PAIR);
    if (!decode(node, rhs))
      return Unexpected(node, ErrorMsg::BAD_CONVERSION);
    return {};
  }
#endif
};

// binary
template <>
struct convert<Binary> {
  static Node encode(const Binary& rhs) {
    return Node(EncodeBase64(rhs.data(), rhs.size()));
  }

  static bool decode(const Node& node, Binary& rhs) {
    if (!node.IsScalar())
      return false;

    std::vector<unsigned char> data = DecodeBase64(node.Scalar());
    if (data.empty() && !node.Scalar().empty())
      return false;

    rhs.swap(data);
    return true;
  }
#if __cplusplus > 202002L
  static Expected<void> expect(const Node& node, Binary& rhs) {
    if (!node.IsDefined())
      return std::unexpected(Exception(node.Mark(), ErrorMsg::INVALID_NODE));
    if (!decode(node, rhs))
      return std::unexpected(Exception(node.Mark(), ErrorMsg::BAD_CONVERSION));
    return {};
  }
#endif
};
}

#endif  // NODE_CONVERT_H_62B23520_7C8E_11DE_8A39_0800200C9A66
