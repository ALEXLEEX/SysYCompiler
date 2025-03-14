#include "type.hpp"

#include "common.hpp"

bool PrimitiveType::equals(const TypePtr& other) const {
  // 先把待考证的other类型转换为PrimitiveType类型
  auto other_type = std::dynamic_pointer_cast<PrimitiveType>(other);
  // 转换不成功则说明other不是PrimitiveType类型
  // 转换成功但是basic_type不相等则说明一个是int一个是void
  return other_type && basic_type == other_type->basic_type;
}

bool ArrayType::equals(const TypePtr& other) const {
  // 判断两个数组类型是否相等
  // tips: 先判断 other 是否是 ArrayType，然后逐个比较 element_type 和 dims
  // 不懂2 这里是指传参的时候省略第 0 维吗
  // tips: 对于 dims，或许可以忽略第 0 维
  // 第 0 维用 -1 表示空
  auto other_type = std::dynamic_pointer_cast<ArrayType>(other);
  if (!other_type || !element_type->equals(other_type->element_type) ||
      dims.size() != other_type->dims.size()) {
    return false;
  }
  for (size_t i = 0; i < dims.size(); i++) {
    if (dims[i] != other_type->dims[i]) {
      return false;
    }
  }
  return true;
// #warning Not implemented: ArrayType::equals
}

bool FuncType::equals(const TypePtr& other) const {
  auto other_type = std::dynamic_pointer_cast<FuncType>(other);
  if (!other_type || !return_type->equals(other_type->return_type) ||
      param_types.size() != other_type->param_types.size()) {
    return false;
  }
  for (size_t i = 0; i < param_types.size(); i++) {
    if (!param_types[i]->equals(other_type->param_types[i])) {
      return false;
    }
  }
  return true;
}
