#include "symbol_table.hpp"

SymbolPtr SymbolTable::add_symbol(std::string name, TypePtr type) {
  // 实现符号表的插入操作
  // 并设置 symbol 的 unique_name 属性（你也可以等到 IR Translation 阶段再设置）
  // 对于局部变量和数组，最好为该标识符重新生成一个唯一名称
  // 对于全局变量和函数，直接使用原名称即可
  // 最后，如果插入成功，返回新的符号
  // 如果符号已经存在，返回 nullptr

#warning Not implemented: SymbolTable::add_symbol
  return nullptr;
}

SymbolPtr SymbolTable::find_symbol(std::string name,
                                   bool in_current_scope) const {
  // 实现符号表的查找操作
  // 找到了返回对应的符号，否则返回 nullptr
  // in_current_scope 为 true 时，只在当前作用域查找

#warning Not implemented: SymbolTable::find_symbol
  return nullptr;
}

void SymbolTable::enter_scope() {
  // 实现符号表的进入作用域操作
  // 需要创建一个新的作用域

#warning Not implemented: SymbolTable::enter_scope
}

void SymbolTable::exit_scope() {
  // 实现符号表的退出作用域操作
  // 需要删除当前作用域中的所有符号

#warning Not implemented: SymbolTable::exit_scope
}
