#include "ir_translator.hpp"

#include <cassert>

std::string IRTranslator::new_temp() {
  static int temp_count = 0;
  return "T" + std::to_string(temp_count++);
}

std::string IRTranslator::new_label() {
  static int label_count = 0;
  return "L" + std::to_string(label_count++);
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

  // added
  // TRANSLATE_NODE(IfStmt)
  // TRANSLATE_NODE(WhileStmt)
  // TRANSLATE_NODE(ExpStmt)
  // TRANSLATE_NODE(PrimaryExp)
  // end added
// #warning Add more AST node types if needed

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

// #warning Add more AST node types if needed

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
  // 1) Insert IR::Function
  ir.push_back(IR::Function::create(node->name));
  // 2) If you have function parameters, insert IR::Param for each parameter
  if (node->params.size() > 0) {
    for (auto &param : node->params) {
      ir.push_back(IR::Param::create(param->ident));
    }
  }
  // 3) Translate the function body
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

// 辅助函数 参照doCheckArrayInit补齐初值列表
std::vector<int> IRTranslator::fillupGlobalInitList(
  const std::vector<AST::InitValPtr>& sublist,
  const std::vector<int>& dims) {
  
  std::vector<int> result;
  int totalCount = 1;
  // 初值列表元素总数
  for (auto d : dims) totalCount *= d;
  int filledCount = 0;
  for (auto child : sublist) {
    auto initv = std::dynamic_pointer_cast<AST::InitVal>(child);
    if (initv->kind == AST::InitVal::Kind::EXP) {
      filledCount++;
      // 单个数组元素初值
      
      // 全局变量的初值一定是INT_CONST
      auto initVal = initv->expr;
      int initVal_value = std::dynamic_pointer_cast<AST::IntConst>(initVal)->value;
      result.push_back(initVal_value);
    } else {
      // 递归处理子列表
      int i = 0;
      int temp = 1;
      for (i = dims.size() - 1; i >= 1; i--) {
        temp *= dims[i];
        if (filledCount % temp != 0) {
          break;
        }
      }
      i++;
      
      // 构建子数组的dims vector 递归调用本函数
      std::vector<int> subDims;
      for (std::vector<int>::size_type j = i; j < dims.size(); j++) {
        subDims.push_back(dims[j]);
      }
      // 递归调用
      auto sublist = initv->subInitVals;
      auto subResult = fillupGlobalInitList(sublist, subDims);
      // 将子列表的初值添加到结果列表中
      result.insert(result.end(), subResult.begin(), subResult.end());
      filledCount += subResult.size();
    }
  }
  // 如果 filledCount < totalCount => 填充0
  if (filledCount < totalCount) {
    for (int i = filledCount; i < totalCount; i++) {
      result.push_back(0);
    }
  }
  return result;
}

IR::Code IRTranslator::fillupLocalInitList(
  const std::vector<AST::InitValPtr>& sublist,
  const std::vector<int>& dims,
  const std::string& place, // 存储数组的名字
  int filledCount // 递归调用时传入的已填充元素个数
) {
  
  // std::vector<int> result;
  IR::Code result;
  int totalCount = 1;
  int nowCount  = 0; // 本次的Count
  // 初值列表元素总数
  for (auto d : dims) totalCount *= d;
  for (auto child : sublist) {
    auto initv = std::dynamic_pointer_cast<AST::InitVal>(child);
    if (initv->kind == AST::InitVal::Kind::EXP) {
      filledCount++;
      nowCount++;
      // 单个数组元素初值
      std::string temp = new_temp();
      auto ir_exp = translateExp(initv->expr, temp);
      std::move(ir_exp.begin(), ir_exp.end(), std::back_inserter(result));
      result.push_back(
        IR::Store::create(place, (filledCount - 1) * 4, temp));
    } else {
      // 递归处理子列表
      int i = 0;
      int temp = 1;
      for (i = dims.size() - 1; i >= 1; i--) {
        temp *= dims[i];
        if (nowCount % temp != 0) {
          break;
        }
      }
      i++;
      
      // 构建子数组的dims vector 递归调用本函数
      std::vector<int> subDims;
      for (std::vector<int>::size_type j = i; j < dims.size(); j++) {
        subDims.push_back(dims[j]);
      }
      // 递归调用
      auto sublist = initv->subInitVals;
      auto subResult = fillupLocalInitList(sublist, subDims, place, filledCount);
      // 将子列表的初值添加到结果列表中
      int subDimsSize = 1;
      for (auto subDim : subDims) {
        subDimsSize *= subDim;
      }
      filledCount += subDimsSize;
      nowCount += subDimsSize;
      std::move(subResult.begin(), subResult.end(), std::back_inserter(result));
    }
  }
  // 如果 filledCount < totalCount => 填充0
  if (nowCount < totalCount) {
    for (int i = nowCount; i < totalCount; i++) {
      // result.push_back(0);
      auto temp = new_temp();
      result.push_back(IR::LoadImm::create(temp, 0));
      result.push_back(
        IR::Store::create(place, (i + filledCount - 1) * 4, temp));
    }
    filledCount += totalCount - nowCount;
    nowCount = totalCount;
  }
  return result;
}

IR::Code IRTranslator::translateVarDef(AST::VarDefPtr node) {
  IR::Code ir;
  // 添加变量定义指令
  // 如果有初始化表达式，则需要翻译初始化表达式
  // 可以用语义分析阶段挂在 VarDef 上的 symbol 来获取变量的类型以及唯一名称

  // if global var or array => IR::Global
  // else local array => IR::Dec
  // node->symbol->scope_index == global or local

  // if node->symbol->is_global then:
  //    code.push_back(IR::Global::create(name, bytes, maybe initVals));
  // else if array => code.push_back(IR::Dec::create(name, bytes));
  // then if there's initVal => generate store instructions

  // no actual code if it's a single int local variable with no init

  if (node->symbol->scope_index == 0) {
    // global
    if (node->dims.empty()) {
      if (node->initVal) {
        // 首先用DEC声明全局变量 全局变量的初值一定是INT_CONST
        auto initVal = node->initVal->expr;
        int initVal_value = std::dynamic_pointer_cast<AST::IntConst>(initVal)->value;
        // 这里应该是一个全局变量的赋值
        ir.push_back(IR::Global::create(node->ident, 4, {initVal_value}));
        // ir.push_back(IR::Global::create(node->ident, 4, {node->initVal->value}));
      } else {
        // 如果没有初值 默认值为0
        ir.push_back(IR::Global::create(node->ident, 4, {0}));
      }
    } else {
      // 全局数组
      // 这里应该在InitVal把数组初始化值补齐
      auto initList = fillupGlobalInitList(node->initVal->subInitVals, node->dims);
      int bytes = 4;
      for (auto d : node->dims) {
        bytes *= d;
      }
      // 这里应该是一个全局数组的列表初始化
      ir.push_back(IR::Global::create(node->ident, bytes, initList));
    //   ir.push_back(IR::Global::create(node->ident, node->symbol->type->size * node->dims[0]));
    }
  } else {
    // local
    if (node->dims.empty()) {
      if (node->initVal) {
        // 局部变量的初值如果有的话 可能是表达式 需要用临时变量先存储表达式的值
        auto exp = node->initVal->expr;
        // 用唯一名称作为临时变量的名字
        auto temp = node->symbol->unique_name;
        auto exp_ir = translateExp(exp, temp);
        std::move(exp_ir.begin(), exp_ir.end(), std::back_inserter(ir));
    //   ir.push_back(IR::Dec::create(node->ident, node->symbol->type->size));
      } else {
        // 局部变量没有初值 暂不做处理
      }
    } else {
      // 局部数组
      auto initList = fillupLocalInitList(node->initVal->subInitVals, node->dims, node->symbol->unique_name, 0);
      int bytes = 4;
      for (auto d : node->dims) {
        bytes *= d;
      }
      ir.push_back(IR::Dec::create(node->symbol->unique_name, bytes));
      std::move(initList.begin(), initList.end(), std::back_inserter(ir));
    }
  }
// #warning Not implemented: IRTranslator::translateVarDef
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
