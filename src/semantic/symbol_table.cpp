#include "symbol_table.hpp"

SymbolPtr SymbolTable::add_symbol(std::string name, TypePtr type) {
  // 实现符号表的插入操作
  // 并设置 symbol 的 unique_name 属性（你也可以等到 IR Translation 阶段再设置）
  // 对于局部变量和数组，最好为该标识符重新生成一个唯一名称
  // 对于全局变量和函数，直接使用原名称即可
  // 最后，如果插入成功，返回新的符号
  // 如果符号已经存在，返回 nullptr
  ASSERT(!scope_stack.empty(), "SymbolTable: no scope is available in add_symbol"); // 断言：作用域栈不为空
  auto &current_scope = scope_stack.back(); // 获取当前作用域
  if (current_scope.find(name) != current_scope.end()) { // 如果当前作用域中已经存在该符号
    return nullptr; // 返回 nullptr
  }
  SymbolPtr symbol = Symbol::create(name, type); // 创建一个新的符号
  if (scope_stack.size() == 1) {
    // 如果是全局作用域
    symbol->unique_name = name; // 直接使用原名称
  } else {
    // 如果是局部作用域 生成 name_n 这样的名称
    symbol->unique_name = name + "_" + std::to_string(local_count++); // 为该标识符重新生成一个唯一名称
  }
  current_scope[name] = symbol; // 将该符号插入到当前作用域
  return symbol; // 返回新的符号
// #warning Not implemented: SymbolTable::add_symbol
}

SymbolPtr SymbolTable::find_symbol(std::string name,
                                   bool in_current_scope) const {
  // 实现符号表的查找操作
  // 找到了返回对应的符号，否则返回 nullptr
  // in_current_scope 为 true 时，只在当前作用域查找
  if (in_current_scope) { // 如果只在当前作用域查找
    ASSERT(!scope_stack.empty(), "SymbolTable: no scope is available in find_symbol"); // 断言：作用域栈不为空
    auto &current_scope = scope_stack.back(); // 获取当前作用域
    if (current_scope.find(name) != current_scope.end()) { // 如果当前作用域中存在该符号
      return current_scope.at(name); // 返回该符号
    }
    return nullptr; // 否则返回 nullptr
  }
  // 否则在所有作用域中查找
  for (auto it = scope_stack.rbegin(); it != scope_stack.rend(); it++) { // 遍历作用域栈
    if (it->find(name) != it->end()) { // 如果当前作用域中存在该符号
      return it->at(name); // 返回该符号
    }
  }
  return nullptr; // 否则返回 nullptr
// #warning Not implemented: SymbolTable::find_symbol
}

void SymbolTable::enter_scope() {
  // 实现符号表的进入作用域操作
  // 需要创建一个新的作用域
  scope_stack.push_back(std::unordered_map<std::string, SymbolPtr>()); // 创建一个新的作用域
// #warning Not implemented: SymbolTable::enter_scope
}

void SymbolTable::exit_scope() {
  // 实现符号表的退出作用域操作
  // 需要删除当前作用域中的所有符号
  ASSERT(!scope_stack.empty(), "SymbolTable: no scope is available in exit_scope"); // 断言：作用域栈不为空
  scope_stack.pop_back(); // 删除当前作用域
// #warning Not implemented: SymbolTable::exit_scope
}
