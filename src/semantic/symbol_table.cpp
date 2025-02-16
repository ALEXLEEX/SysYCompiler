#include "symbol_table.hpp"

Symbol::Symbol(std::string name, TypePtr type) : name(name), type(type) {
  // 实现构造函数
  // 对于非函数类型的符号，需要为其生成唯一名称

#warning Not implemented: Symbol::Symbol
}

std::string SymbolTable::add_symbol(SymbolPtr symbol) {
  // 实现符号表的插入操作
  // 对于非函数类型的符号，最好返回为该标识符生成的唯一名称
  // 这样在后续 IR Translation 阶段就不需要考虑 scope 的问题

#warning Not implemented: SymbolTable::add_symbol
}

SymbolPtr SymbolTable::find_symbol(std::string name) const {
  // 实现符号表的查找操作
  // 找到了返回对应的符号，否则返回 nullptr

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
