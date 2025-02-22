#ifndef SEMANTIC_SYMBOL_TABLE_HPP
#define SEMANTIC_SYMBOL_TABLE_HPP

#include <memory>
#include <string>
#include <unordered_map>
#include <vector>

#include "type.hpp"

class Symbol;
using SymbolPtr = std::shared_ptr<Symbol>;
class Symbol {
 public:
  /// @brief The name of the symbol
  std::string name;
  /// @brief The unique name of the symbol
  std::string unique_name;
  /// @brief The type of the symbol
  TypePtr type;
  Symbol(std::string name, TypePtr type) : name(name), type(type) {}
  static SymbolPtr create(std::string name, TypePtr type) {
    return std::make_shared<Symbol>(name, type);
  }
};

class SymbolTable {
 public:
  /// @brief Add a symbol to the table and return the unique name of the symbol
  /// @param symbol The symbol to add
  /// @return The unique name of the symbol
  std::string add_symbol(SymbolPtr symbol);

  /// @brief Add a symbol to the table and return the unique name of the symbol
  /// @param name The name of the symbol
  /// @param type The type of the symbol
  /// @return The unique name of the symbol
  std::string add_symbol(std::string name, TypePtr type) {
    return add_symbol(Symbol::create(name, type));
  }

  /// @brief Find a symbol by name
  /// @param name The name of the symbol
  /// @return The symbol if found, nullptr otherwise
  SymbolPtr find_symbol(std::string name) const;

  /// @brief Enter a new scope
  void enter_scope();

  /// @brief Exit the current scope
  void exit_scope();

 private:
#warning Not implemented: SymbolTable
};

#endif  // SEMANTIC_SYMBOL_TABLE_HPP