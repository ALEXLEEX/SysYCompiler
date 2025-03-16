#ifndef SEMANTIC_TYPE_HPP
#define SEMANTIC_TYPE_HPP

#include <iostream>
#include <string>
#include <vector>

#include "common.hpp"

// 类型基类 Type
class Type;
using TypePtr = std::shared_ptr<Type>;
class Type {
 public:
  // 类型基类 Type 有两个纯虚函数 equals 和 to_string，分别用于比较两个类型是否相等和将类型转换为字符串。
  virtual bool equals(const TypePtr& other) const = 0;
  virtual std::string to_string() const = 0;
  virtual ~Type() = default;  // make the class polymorphic 多态
};

// 基本类型 PrimitiveType
// enum class BasicType { Unknown, Int, Void };
class PrimitiveType;
using PrimitiveTypePtr = std::shared_ptr<PrimitiveType>;
class PrimitiveType : public Type {
 public:
  // 基本类型 BasicType 用于表示整型和空类型
  BasicType basic_type;

  PrimitiveType(BasicType basic_type) : basic_type(basic_type) {}

  // 通过静态成员函数 create 创建 PrimitiveType 对象
  static PrimitiveTypePtr create(BasicType basic_type) {
    return std::make_shared<PrimitiveType>(basic_type);
  }

  bool equals(const TypePtr& other) const override;

  std::string to_string() const override { return type_to_string(basic_type); }

  static const TypePtr Int;
  static const TypePtr Void;
};

// 就是可以直接通过类名调用 不用每次都create::(BasicType::Int)这样了
// 为了方便使用，我们在 PrimitiveType 类中定义了两个静态成员变量 Int 和 Void，分别表示整型和空类型。
inline const TypePtr PrimitiveType::Int = PrimitiveType::create(BasicType::Int);
inline const TypePtr PrimitiveType::Void = PrimitiveType::create(BasicType::Void);

class ArrayType;
using ArrayTypePtr = std::shared_ptr<ArrayType>;
class ArrayType : public Type {
 public:
  // 现在element_type只可能是int类型 但是这里放一个指针是为了以后方便扩展
  TypePtr element_type;
  std::vector<int> dims;

  ArrayType() = default;
  ArrayType(TypePtr element_type, std::vector<int> dims)
      : element_type(element_type), dims(dims) {
    ASSERT(dims.size() > 0, "Array dimension should be greater than 0");
  }

  static ArrayTypePtr create(TypePtr element_type, std::vector<int> dims) {
    return std::make_shared<ArrayType>(element_type, dims);
  }

  bool equals(const TypePtr& other) const override;

  std::string to_string() const override {
    std::string result = element_type->to_string() + " (*)";
    for (size_t i = 0; i < dims.size(); i++) {
      result += "[" + std::to_string(dims[i]) + "]";
    }
    return result;
  }
};

/*
如果需要额外类型 如省略第一维的形参数组
可以在 type.hpp / type.cpp 中扩展/修改你现有的 ArrayType / FunctionType 结构
*/

class FuncType;
using FuncTypePtr = std::shared_ptr<FuncType>;
class FuncType : public Type {
 public:
  // 估计这里就是要return_type去等于PrimitiveType::Int或者PrimitiveType::Void之中某一个
  TypePtr return_type;
  std::vector<TypePtr> param_types;

  FuncType() = default;
  FuncType(TypePtr return_type, std::vector<TypePtr> param_types)
      : return_type(return_type), param_types(param_types) {}

  static FuncTypePtr create(TypePtr return_type,
                            std::vector<TypePtr> param_types) {
    return std::make_shared<FuncType>(return_type, param_types);
  }

  bool equals(const TypePtr& other) const override;

  std::string to_string() const override {
    std::string result = return_type->to_string() + " (*)(";
    for (size_t i = 0; i < param_types.size(); i++) {
      result += param_types[i]->to_string();
      if (i + 1 < param_types.size()) {
        result += ", ";
      }
    }
    return result + ")";
  }
};

#endif  // SEMANTIC_TYPE_HPP