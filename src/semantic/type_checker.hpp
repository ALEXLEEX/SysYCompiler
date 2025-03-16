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

  // 只允许外部调用 check 方法
  // check 方法会根据 AST 的根节点类型调用对应的 check 方法
  // 从而递归地对 AST 进行类型检查
  TypePtr check(AST::NodePtr node);

 private:
  /// @brief The symbol table
  SymbolTable symbol_table;

  // to store current function return type
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
  TypePtr checkPrimaryExp(AST::PrimaryExpPtr node);
  TypePtr checkInitVal(AST::InitValPtr node, const TypePtr& target_type);
  void doCheckArrayInit(
    const std::vector<AST::InitValPtr> & sublist, 
    const std::shared_ptr<ArrayType> & arrType,
    int initializedCount,
    int lineno
  );
  //
  TypePtr checkVarDef(AST::VarDefPtr node, BasicType var_type);
  TypePtr checkVarDecl(AST::VarDeclPtr node);
  TypePtr checkFuncDef(AST::FuncDefPtr node);
  TypePtr checkCompUnit(AST::CompUnitPtr node);
};

#endif  // SEMANTIC_TYPE_CHECKER_HPP