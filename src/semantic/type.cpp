#include "type.hpp"

#include "common.hpp"

bool PrimitiveType::equals(const TypePtr& other) const {
  auto other_type = std::dynamic_pointer_cast<PrimitiveType>(other);
  return other_type && basic_type == other_type->basic_type;
}

bool ArrayType::equals(const TypePtr& other) const {
  // 判断两个数组类型是否相等
  // tips: 先判断 other 是否是 ArrayType，然后逐个比较 element_type 和 dims
  // tips: 对于 dims，或许可以忽略第 0 维

#warning Not implemented: ArrayType::equals
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
