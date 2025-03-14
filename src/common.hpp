#ifndef COMMON_HPP
#define COMMON_HPP
// 存放整个项目中通用的定义和工具 方便在不同模块间共享和复用代码
#include <iostream>
#include <memory>
#include <string>

// 基本类型枚举 仅支持 Int 和 Void 两种类型
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
 * 把二元/一元等运算都统一在这里 需要在后续解析/AST处理时加以区分 
 * 也就是说构建 Unary Op 的时候传的还是一个 BinaryOp 枚举 
 */
enum class BinaryOp {
  // 算术
  Add,  // + 二元/一元
  Sub,  // - 二元/一元
  Mul,  // *
  Div,  // /
  Mod,  // %
  // 逻辑一元
  Not,  // ! 一元
  // 逻辑二元
  LAnd, // &&
  LOr,  // ||
  // 比较
  EQ,   // ==
  NEQ,  // !=
  LT,   // <
  LE,   // <=
  GT,   // >
  GE,   // >=
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
// 用于判断一个 shared_ptr 是否是某个类型
// 将 U 类型的 shared_ptr 转换为 T 类型的 shared_ptr
// 如果转换成功则说明 U 是 T 的派生类
// 用法示例: type_of<BinaryExp>(node)
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
