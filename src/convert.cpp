#include <algorithm>
#include <array>

#include "yaml-cpp/node/convert.h"
#include "yaml-cpp/exceptions.h"

namespace {
// we're not gonna mess with the mess that is all the isupper/etc. functions
bool IsLower(char ch) { return 'a' <= ch && ch <= 'z'; }
bool IsUpper(char ch) { return 'A' <= ch && ch <= 'Z'; }
char ToLower(char ch) { return IsUpper(ch) ? ch + 'a' - 'A' : ch; }

std::string tolower(const std::string& str) {
  std::string s(str);
  std::transform(s.begin(), s.end(), s.begin(), ToLower);
  return s;
}

template <typename T>
bool IsEntirely(const std::string& str, T func) {
  return std::all_of(str.begin(), str.end(), [=](char ch) { return func(ch); });
}

// IsFlexibleCase
// . Returns true if 'str' is:
//   . UPPERCASE
//   . lowercase
//   . Capitalized
bool IsFlexibleCase(const std::string& str) {
  if (str.empty())
    return true;

  if (IsEntirely(str, IsLower))
    return true;

  bool firstcaps = IsUpper(str[0]);
  std::string rest = str.substr(1);
  return firstcaps && (IsEntirely(rest, IsLower) || IsEntirely(rest, IsUpper));
}
}  // namespace

namespace YAML {
bool convert<bool>::decode(const Node& node, bool& rhs) {
  if (!node.IsScalar())
    return false;

  // we can't use iostream bool extraction operators as they don't
  // recognize all possible values in the table below (taken from
  // http://yaml.org/type/bool.html)
  static const struct {
    std::string truename, falsename;
  } names[] = {
      {"y", "n"},
      {"yes", "no"},
      {"true", "false"},
      {"on", "off"},
  };

  if (!IsFlexibleCase(node.Scalar()))
    return false;

  for (const auto& name : names) {
    if (name.truename == tolower(node.Scalar())) {
      rhs = true;
      return true;
    }

    if (name.falsename == tolower(node.Scalar())) {
      rhs = false;
      return true;
    }
  }

  return false;
}

#if __cplusplus > 202002L
template<>
Expected<bool> expect<bool>::operator()(const Node& node) const noexcept
{
  if (!node.IsScalar()) {
    return Unexpected(node.Mark(), ErrorMsg::NOT_A_BOOL);
  }
  if (!isFlexibleCase(node.Scalar())) {
    return Unexpected(node.Mark(), ErrorMsg::NOT_FLEXIBLE_BOOL);
  }
  const std::string lower = tolower(node.Scalar());
  auto is_match = [&lower](auto const & str) {
      return lower == str;
  };
  if (std::ranges::any_of(std::array{"y", "yes", "true", "on"}, is_match)) {
      return true;
  }
  if (std::ranges::any_of(std::array{"n", "no", "false", "off"}, is_match)) {
      return false;
  }
  return Unexpected(node.Mark(), ErrorMsg::NOT_A_BOOL);
}
#endif
}  // namespace YAML
