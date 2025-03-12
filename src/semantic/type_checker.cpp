#include "type_checker.hpp"

#include "common.hpp"

TypeChecker::TypeChecker() {
  // 你需要在这里对 symbol_table 进行初始化
  // 插入一些内置函数，如 read 和 write

#warning Not implemented: TypeChecker::TypeChecker
}

TypePtr TypeChecker::check(AST::NodePtr node) {
#define CHECK_NODE(type)                                     \
  if (auto n = std::dynamic_pointer_cast<AST::type>(node)) { \
    return check##type(n);                                   \
  }

  // 递归检查 AST 的每个节点
  // 如果你添加了新的 AST 节点类型，记得在这里添加对应的检查函数
  CHECK_NODE(CompUnit)
  CHECK_NODE(FuncDef)
  CHECK_NODE(VarDecl)
  CHECK_NODE(Block)
  CHECK_NODE(AssignStmt)
  CHECK_NODE(ReturnStmt)
  CHECK_NODE(LVal)
  CHECK_NODE(IntConst)
  CHECK_NODE(FuncCall)
  CHECK_NODE(UnaryExp)
  CHECK_NODE(BinaryExp)

#warning Add more AST node types if needed

#undef CHECK_NODE

  ASSERT(false, "Unknown AST node type " + node->to_string() +
                    " in type checking at line " +
                    std::to_string(node->lineno));
}

TypePtr TypeChecker::checkCompUnit(AST::CompUnitPtr node) {
  for (auto &unit : node->units) {
    check(unit);
  }
  return nullptr;
}

TypePtr TypeChecker::checkFuncDef(AST::FuncDefPtr node) {
  // 在这个函数中，你需要判断函数是否已经被定义过
  // 如果函数已经被定义过，你需要报错
  // 否则，你需要将函数插入符号表，并在符号表中创建一个新的作用域
  // 再将函数参数也插入符号表，并将符号表中对应的 symbol 挂到 FuncDef 节点上
  // 最后检查函数体的语句块

#warning Not implemented: TypeChecker::checkFuncDef
  return nullptr;
}

TypePtr TypeChecker::checkVarDecl(AST::VarDeclPtr node) {
  for (auto var_def : node->defs) {
    checkVarDef(var_def, node->btype);
  }
  return nullptr;
}

TypePtr TypeChecker::checkVarDef(AST::VarDefPtr node, BasicType var_type) {
  // 你需要判断变量是否已经被定义过，并更新符号表
  auto type = PrimitiveType::create(var_type);
  // 判断变量是否已经被定义过
  // 如果有初始化表达式，你需要检查初始化表达式的类型是否和变量类型相同
  // 如果是数组，你还需要检查初始化表达式和数组的维度是否匹配，是否有溢出的情况

#warning Not implemented: TypeChecker::checkVarDef

  // 将变量插入符号表，并将符号表中的 symbol 挂到 VarDef 节点上
  node->symbol = symbol_table.add_symbol(node->ident, type);
  return nullptr;
}

TypePtr TypeChecker::checkBlock(AST::BlockPtr node, bool new_scope) {
  // 检查块内的每个语句
  // 如果 new_scope 为 true
  // 你需要在进入和退出块时更新符号表，创建、销毁新的作用域

#warning Not implemented: TypeChecker::checkBlock
  return nullptr;
}

TypePtr TypeChecker::checkAssignStmt(AST::AssignStmtPtr node) {
  TypePtr lval_type = check(node->lval);
  TypePtr expr_type = check(node->exp);
  // 判断赋值号两边的类型是否相同
  // 我们实验中只支持 int 类型
  // 因此你需要判断 lval_type 和 expr_type 是否都为 int 类型

#warning Not implemented: TypeChecker::checkAssignStmt
  return lval_type;
}

TypePtr TypeChecker::checkReturnStmt(AST::ReturnStmtPtr node) {
  TypePtr expr_type = check(node->exp);
  // 判断返回值类型是否和函数声明的返回值类型相同

#warning Not implemented: TypeChecker::checkReturnStmt
  return nullptr;
}

TypePtr TypeChecker::checkLVal(AST::LValPtr node) {
  // 你需要在这里查找符号表，判断变量是否被定义过
  // 根据符号表中的信息设置 LVal 的类型
  // 若变量未定义，你需要报错
  // 否则，将符号表中的 symbol 挂到 LVal 节点上
  // 如果 LVal 是数组，你还需要根据下标索引来设置 LVal 的类型

#warning Not implemented: TypeChecker::checkLVal
  // 你需要返回 LVal 的类型
  return nullptr;
}

TypePtr TypeChecker::checkIntConst(AST::IntConstPtr node) {
  // 整数常量的类型是 int
  return PrimitiveType::Int;
}

TypePtr TypeChecker::checkFuncCall(AST::FuncCallPtr node) {
  // 首先需要查找函数是否被定义过
  // 然后需要判断函数调用的参数个数和类型是否和声明一致
  // 最后设置函数调用表达式的类型为函数的返回值类型
  // 并将函数的 symbol 挂到 FuncCall 节点上

#warning Not implemented: TypeChecker::checkFuncCall
  // 你需要返回函数调用表达式的类型
  return nullptr;
}

TypePtr TypeChecker::checkUnaryExp(AST::UnaryExpPtr node) {
  auto type = check(node->exp);
  // 一元表达式只支持 int 类型，因此你需要判断 type 是否为 int

#warning Not implemented: TypeChecker::checkUnaryExp
  return PrimitiveType::Int;
}

TypePtr TypeChecker::checkBinaryExp(AST::BinaryExpPtr node) {
  TypePtr left_type = check(node->left);
  TypePtr right_type = check(node->right);
  // 二元表达式只支持 int 类型，因此你需要判断左右表达式的类型是否为 int

#warning Not implemented: TypeChecker::checkBinaryExp
  return PrimitiveType::Int;
}
