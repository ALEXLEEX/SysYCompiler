#ifndef COMMON_HPP
#define COMMON_HPP
// 存放整个项目中通用的定义和工具，方便在不同模块间共享和复用代码
#include <iostream>
#include <memory>
#include <string>

// 基本类型枚举
enum class BasicType { Unknown, Int, Void };
inline constexpr const char *type_to_string(BasicType type) {
  switch (type) {
    case BasicType::Unknown:
      return "unknown";
    case BasicType::Int:
      return "int";
    case BasicType::Void:
      return "void";
  }
  return "unknown";
}

/**
 * 把二元/一元等运算都统一在这里的话，需要在后续解析/AST处理时加以区分。
 * 也可以再弄一个 enum class UnaryOp { ... } 做一元运算，取决于实现。
 */
enum class BinaryOp {
  // 算术
  Add,  // +
  Sub,  // -
  Mul,  // *
  Div,  // /
  Mod,  // %
  // 逻辑一元运算
  Not,  // ! (一元)
  // 逻辑二元运算
  LAnd, // &&
  LOr,  // ||
  // 比较
  EQ,   // ==
  NEQ,  // !=
  LT,   // <
  LE,   // <=
  GT,   // >
  GE,   // >=

  // 如果你只打算把一元正负号等价放在这里，也可以，但最好注意区分
  // e.g. "一元正号" vs "二元加法"
};

inline constexpr const char *op_to_string(BinaryOp op) {
  switch (op) {
    case BinaryOp::Add:   return "+";
    case BinaryOp::Sub:   return "-";
    case BinaryOp::Mul:   return "*";
    case BinaryOp::Div:   return "/";
    case BinaryOp::Mod:   return "%";
    case BinaryOp::Not:   return "!";
    case BinaryOp::LAnd:  return "&&";
    case BinaryOp::LOr:   return "||";
    case BinaryOp::EQ:    return "==";
    case BinaryOp::NEQ:   return "!=";
    case BinaryOp::LT:    return "<";
    case BinaryOp::LE:    return "<=";
    case BinaryOp::GT:    return ">";
    case BinaryOp::GE:    return ">=";
  }
  return "unknown";
}

// use C++ RTTI to check the type of a shared_ptr
template <typename T, typename U>
inline bool type_of(const std::shared_ptr<U> &node) {
  return std::dynamic_pointer_cast<T>(node) != nullptr;
}

#define ASSERT(expr, msg)                                                \
  do {                                                                   \
    if (!(expr)) {                                                       \
      std::cerr << "Assertion failed at " << __FILE__ << ":" << __LINE__ \
                << " (" << #expr << "): " << msg << std::endl;           \
      std::exit(1);                                                      \
    }                                                                    \
  } while (0)

#endif  // COMMON_HPP
