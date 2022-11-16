#include "yaml-cpp/exceptions.h"
#include "yaml-cpp/noexcept.h"
#include "yaml-cpp/node/node.h"

#if __cplusplus > 202002L
#include <expected>
#endif

namespace YAML {

// These destructors are defined out-of-line so the vtable is only emitted once.
Exception::~Exception() YAML_CPP_NOEXCEPT = default;
ParserException::~ParserException() YAML_CPP_NOEXCEPT = default;
RepresentationException::~RepresentationException() YAML_CPP_NOEXCEPT = default;
InvalidScalar::~InvalidScalar() YAML_CPP_NOEXCEPT = default;
KeyNotFound::~KeyNotFound() YAML_CPP_NOEXCEPT = default;
InvalidNode::~InvalidNode() YAML_CPP_NOEXCEPT = default;
BadConversion::~BadConversion() YAML_CPP_NOEXCEPT = default;
BadDereference::~BadDereference() YAML_CPP_NOEXCEPT = default;
BadSubscript::~BadSubscript() YAML_CPP_NOEXCEPT = default;
BadPushback::~BadPushback() YAML_CPP_NOEXCEPT = default;
BadInsert::~BadInsert() YAML_CPP_NOEXCEPT = default;
EmitterException::~EmitterException() YAML_CPP_NOEXCEPT = default;
BadFile::~BadFile() YAML_CPP_NOEXCEPT = default;

#if __cplusplus > 202002L
std::unexpected<Exception>
Unexpected(const Mark& mark, const std::string& msg)
{
    return std::unexpected(Exception(mark, msg));
}

std::unexpected<Exception>
Unexpected(const Node& node, const std::string& msg)
{
    return std::unexpected(Exception(node.Mark(), msg));
}
#endif

}  // namespace YAML
