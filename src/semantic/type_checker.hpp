#ifndef SEMANTIC_TYPE_CHECKER_HPP
#define SEMANTIC_TYPE_CHECKER_HPP

#include <memory>

#include "ast/tree.hpp"
#include "symbol_table.hpp"

/**
 * @brief The TypeChecker class is responsible for semantic analysis.
 * It traverses the AST, builds the symbol table, and checks type correctness.
 */
class TypeChecker {
 public:
  TypeChecker();

  TypePtr check(AST::NodePtr node);

 private:
  /// @brief The symbol table
  SymbolTable symbol_table;

  // If you want to store "current function return type" etc., 
  // you can add members here:
  TypePtr current_func_return_type; 


  TypePtr checkIntConst(AST::IntConstPtr node);
  TypePtr checkLVal(AST::LValPtr node);
  TypePtr checkUnaryExp(AST::UnaryExpPtr node);
  TypePtr checkBinaryExp(AST::BinaryExpPtr node);
  TypePtr checkFuncCall(AST::FuncCallPtr node);
  TypePtr checkBlock(AST::BlockPtr node, bool new_scope = true);
  TypePtr checkAssignStmt(AST::AssignStmtPtr node);
  TypePtr checkReturnStmt(AST::ReturnStmtPtr node);
  // added
  TypePtr checkIfStmt(AST::IfStmtPtr node);
  TypePtr checkWhileStmt(AST::WhileStmtPtr node);
  TypePtr checkExpStmt(AST::ExpStmtPtr node);
  //
  TypePtr checkVarDef(AST::VarDefPtr node, BasicType var_type);
  TypePtr checkVarDecl(AST::VarDeclPtr node);
  TypePtr checkFuncDef(AST::FuncDefPtr node);
  TypePtr checkCompUnit(AST::CompUnitPtr node);
};

#endif  // SEMANTIC_TYPE_CHECKER_HPP