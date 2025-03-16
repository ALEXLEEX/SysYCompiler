#include "ir_translator.hpp"

#include <cassert>

std::string IRTranslator::new_temp() {
  static int temp_count = 0;
  return "T" + std::to_string(temp_count++);
}

IR::Code IRTranslator::translate(AST::NodePtr node) {
#define TRANSLATE_NODE(type)                                 \
  if (auto n = std::dynamic_pointer_cast<AST::type>(node)) { \
    return translate##type(n);                               \
  }
  // 递归翻译 AST 的每个节点
  // 如果你添加了新的 AST 节点类型，记得在这里添加对应的翻译函数

  TRANSLATE_NODE(CompUnit)
  TRANSLATE_NODE(FuncDef)
  TRANSLATE_NODE(Block)
  TRANSLATE_NODE(VarDecl)
  TRANSLATE_NODE(VarDef)
  TRANSLATE_NODE(AssignStmt)
  TRANSLATE_NODE(ReturnStmt)
  TRANSLATE_NODE(LVal)
  TRANSLATE_NODE(BinaryExp)
  TRANSLATE_NODE(UnaryExp)
  TRANSLATE_NODE(FuncCall)
  TRANSLATE_NODE(IntConst)

#warning Add more AST node types if needed

#undef TRANSLATE_NODE

  ASSERT(false,
         "Unknown AST node type " + node->to_string() + " in IR translation");
}

IR::Code IRTranslator::translateExp(AST::NodePtr node,
                                    const std::string &place) {
#define TRANSLATE_EXP_NODE(type)                             \
  if (auto n = std::dynamic_pointer_cast<AST::type>(node)) { \
    return translate##type(n, place);                        \
  }

  TRANSLATE_EXP_NODE(BinaryExp)
  TRANSLATE_EXP_NODE(UnaryExp)
  TRANSLATE_EXP_NODE(FuncCall)
  TRANSLATE_EXP_NODE(IntConst)
  TRANSLATE_EXP_NODE(LVal)

#warning Add more AST node types if needed

#undef TRANSLATE_EXP_NODE

  ASSERT(false, "No translateExp for node " + node->to_string());
}

IR::Code IRTranslator::translateCompUnit(AST::CompUnitPtr node) {
  IR::Code ir;
  for (auto &unit : node->units) {
    auto unit_ir = translate(unit);
    std::move(unit_ir.begin(), unit_ir.end(), std::back_inserter(ir));
  }
  return ir;
}

IR::Code IRTranslator::translateFuncDef(AST::FuncDefPtr node) {
  IR::Code ir;
  ir.push_back(IR::Function::create(node->name));
  auto block_ir = translate(node->block);
  std::move(block_ir.begin(), block_ir.end(), std::back_inserter(ir));
  return ir;
}

IR::Code IRTranslator::translateBlock(AST::BlockPtr node) {
  IR::Code ir;
  for (auto &stmt : node->stmts) {
    auto stmt_ir = translate(stmt);
    std::move(stmt_ir.begin(), stmt_ir.end(), std::back_inserter(ir));
  }
  return ir;
}

IR::Code IRTranslator::translateVarDecl(AST::VarDeclPtr node) {
  IR::Code ir;
  for (auto &def : node->defs) {
    auto def_ir = translate(def);
    std::move(def_ir.begin(), def_ir.end(), std::back_inserter(ir));
  }
  return ir;
}

IR::Code IRTranslator::translateVarDef(AST::VarDefPtr node) {
  IR::Code ir;
  // 添加变量定义指令
  // 如果有初始化表达式，则需要翻译初始化表达式
  // 可以用语义分析阶段挂在 VarDef 上的 symbol 来获取变量的类型以及唯一名称

#warning Not implemented: IRTranslator::translateVarDef

  return ir;
}

IR::Code IRTranslator::translateAssignStmt(AST::AssignStmtPtr node) {
  IR::Code ir;
  auto lnode = node->lval;
  auto rnode = node->exp;

  // 翻译左值和右值

#warning Not implemented: IRTranslator::translateAssignStmt

  return ir;
}

IR::Code IRTranslator::translateReturnStmt(AST::ReturnStmtPtr node) {
  IR::Code ir;

  // 翻译返回值
  // 如果有返回值，则：
  // place = new_temp();
  // auto exp_ir = translateExp(node->exp, place);
  // return exp_ir + [RETURN place];
  // 否则：
  // return [RETURN];

#warning Not implemented: IRTranslator::translateReturnStmt

  return ir;
}

IR::Code IRTranslator::translateLVal(AST::LValPtr node,
                                     const std::string &place) {
  IR::Code ir;

  // 如果 place 不为空，则将 LVal 的值赋给 place
  // 如果是数组，你需要特殊考虑

#warning Not implemented: IRTranslator::translateLVal

  return ir;
}

IR::Code IRTranslator::translateBinaryExp(AST::BinaryExpPtr node,
                                          const std::string &place) {
  IR::Code ir;
  auto left_place = new_temp();
  auto right_place = new_temp();

  // 翻译左右子表达式
  auto left_ir = translateExp(node->left, left_place);
  auto right_ir = translateExp(node->right, right_place);

  std::move(left_ir.begin(), left_ir.end(), std::back_inserter(ir));
  std::move(right_ir.begin(), right_ir.end(), std::back_inserter(ir));

  // 添加二元运算指令
  if (!place.empty()) {
    ir.push_back(IR::Binary::create(place, left_place, node->op, right_place));
  }
  return ir;
}

IR::Code IRTranslator::translateUnaryExp(AST::UnaryExpPtr node,
                                         const std::string &place) {
  IR::Code ir;

  // 翻译子表达式
  // 如果 place 不为空，则将 UnaryExp 的值赋给 place

#warning Not implemented: IRTranslator::translateUnaryExp

  return ir;
}

IR::Code IRTranslator::translateFuncCall(AST::FuncCallPtr node,
                                         const std::string &place) {
  IR::Code ir;
  std::vector<std::string> arg_places;

  // 首先翻译参数表达式，并存在临时变量中
  // 接下来，添加参数传递指令和函数调用指令
  // 如果 place 不为空，则将函数调用的返回值赋给 place

#warning Not implemented: IRTranslator::translateFuncCall

  return ir;
}

IR::Code IRTranslator::translateIntConst(AST::IntConstPtr node,
                                         const std::string &place) {
  IR::Code ir;
  // 添加赋值常量指令
  if (!place.empty()) {
    ir.push_back(IR::LoadImm::create(place, node->value));
  }
  return ir;
}
