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
  TRANSLATE_NODE(IfStmt)
  TRANSLATE_NODE(WhileStmt)
  TRANSLATE_NODE(ExpStmt)
  TRANSLATE_NODE(PrimaryExp)
  TRANSLATE_NODE(Param)
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
  // 1) Insert IR::Function 都要用唯一名称
  // ir.push_back(IR::Function::create(node->name));
  ir.push_back(IR::Function::create(node->symbol->unique_name));
  // 2) If you have function parameters, insert IR::Param for each parameter
  if (node->params.size() > 0) {
    for (auto &param : node->params) {
      // ir.push_back(IR::Param::create(param->ident));
      ir.push_back(IR::Param::create(param->symbol->unique_name));
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
      if (node->initVal) {
        // 全局数组有初值
        auto initList = fillupGlobalInitList(node->initVal->subInitVals, node->dims);
        int bytes = 4;
        for (auto d : node->dims) {
          bytes *= d;
        }
        ir.push_back(IR::Global::create(node->ident, bytes, initList));
      } else {
        // 全局数组没有初值 默认值为0 TOOD 这里写的是真的丑 假设没有初值就是设置了 int a[10] = {};
        node->initVal = std::make_shared<AST::InitVal>();
        node->initVal->kind = AST::InitVal::Kind::BRACE;
        auto initList = fillupGlobalInitList(node->initVal->subInitVals, node->dims);
        int bytes = 4;
        for (auto d : node->dims) {
          bytes *= d;
        }
        ir.push_back(IR::Global::create(node->ident, bytes, initList));
      }
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
    } else if (node->initVal) {
      // 局部数组
      IR::Code initList = fillupLocalInitList(node->initVal->subInitVals, node->dims, node->symbol->unique_name, 0);
      int bytes = 4;
      for (auto d : node->dims) {
        bytes *= d;
      }
      ir.push_back(IR::Dec::create(node->symbol->unique_name, bytes));
      std::move(initList.begin(), initList.end(), std::back_inserter(ir));
    } else {
      // 局部数组没有初值 暂不做处理
      int bytes = 4;
      for (auto d : node->dims) {
        bytes *= d;
      }
      ir.push_back(IR::Dec::create(node->symbol->unique_name, bytes));
    }
  }
// #warning Not implemented: IRTranslator::translateVarDef
  return ir;
}

IR::Code IRTranslator::translateAssignStmt(AST::AssignStmtPtr node) {
  // 翻译左值和右值
  // #warning Not implemented: IRTranslator::translateAssignStmt
  IR::Code ir;
  auto lnode = node->lval;
  auto rnode = node->exp;

  // 1) 生成一个临时变量,用于放右值表达式的结果
  std::string rtemp = new_temp();
  // 把右侧表达式翻译到 rtemp
  IR::Code rc = translateExp(rnode, rtemp);
  std::move(rc.begin(), rc.end(), std::back_inserter(ir));
  // 2) 处理左值 lnode
  //   如果不是数组, dims.size()==0 => 直接 Assign
  //   如果是数组 => 计算地址 = base + offset, 再 Store
  auto &indexExps = lnode->dims;  // if it's array, e.g. a[x+1][y]

  if (indexExps.empty()) {
    // not array, just do "baseName = rtemp"
    ir.push_back( IR::Assign::create(lnode->symbol->unique_name, rtemp));
  } else {
    // 2.1) 计算多维下标 => offset
    // 这里我们简化为 "offset = (ExpIndex1 * stride1 + ExpIndex2 * stride2 + ...) * 4"
    // 也可能要看 symbol->type->dims to compute each dimension's product
    // 先假设 1D: a[x], offset = x*4
    // 对多维: e.g. a[i][j], offset = (i*col + j)*4
    std::string baseName = new_temp();
    // 如果是全局数组 要先使用AddressOf获取unique_name
    if (lnode->symbol->scope_index == 0) {
      // global baseName = &unique_name
      ir.push_back(IR::AddressOf::create(baseName, lnode->symbol->unique_name));
    } else {
      // local baseName = unique_name
      ir.push_back(IR::Assign::create(baseName, lnode->symbol->unique_name));
    }

    SymbolPtr sym = node->lval->symbol;
    auto arrType = std::dynamic_pointer_cast<ArrayType>(sym->type);
    std::string idxTemp;  // 临时变量存放下标表达式的值
    for (std::vector<int>::size_type i = 0; i < indexExps.size(); i++) {

      std::string offsetTemp = new_temp();
      // offsetTemp = #1 initially 为连乘做准备
      ir.push_back( IR::LoadImm::create(offsetTemp, 1));

      auto dimExp = indexExps[i];
      int subArrSize = 1;
      // 0) calculate subarray size
      for (std::vector<int>::size_type j = i + 1; j < arrType->dims.size(); j++) {
        subArrSize *= arrType->dims[j];
      }
      // 1) translateExp( dimExp, idxTemp )
      idxTemp = new_temp();
      // idxTemp 存放的是下标表达式的值 = dimExp
      auto exp_ir = translateExp(dimExp, idxTemp);
      std::move(exp_ir.begin(), exp_ir.end(), std::back_inserter(ir));

      // 2) offsetTemp = offsetTemp + <subDimensionSize> * idxTemp
      // 子数组大小 * 4 * 偏移量 就是这个维度的偏移量
      ir.push_back(IR::Binary::create(offsetTemp, idxTemp, BinaryOp::Mul, IR::imm_str(subArrSize * 4)));
      ir.push_back(IR::Binary::create(baseName, baseName, BinaryOp::Add, offsetTemp));
    }
    // 2.2) 现在 offsetTemp 里是字节偏移 且已经加进去了
    // 做 store: "*( baseName + #offsetTemp ) = rtemp"
    ir.push_back(IR::Store::create(baseName, rtemp));
  }
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

  std::string place;
  if (node->exp) {
    place = new_temp();
    auto exp_ir = translateExp(node->exp, place);
    std::move(exp_ir.begin(), exp_ir.end(), std::back_inserter(ir));
    ir.push_back(IR::Return::create(place));
  } else {
    ir.push_back(IR::Return::create());
  }
// #warning Not implemented: IRTranslator::translateReturnStmt
  return ir;
}

IR::Code IRTranslator::translateLVal(AST::LValPtr node,
                                     const std::string &place) {
  IR::Code ir;

  // 如果 place 不为空，则将 LVal 的值赋给 place 此时不是真正的左值
  // 如果是数组，你需要特殊考虑
  if (node->dims.empty()) {
    // 不是数组，直接赋值
    if (!place.empty()) {
      ir.push_back(IR::Assign::create(place, node->symbol->unique_name));
    }
  } else {
    // 是数组，计算地址
    // 这里需要计算偏移量
    // 计算方法和上面类似
    std::string baseName = new_temp();
    // 如果是全局数组 要先使用AddressOf获取unique_name
    if (node->symbol->scope_index == 0) {
      // global baseName = &unique_name
      ir.push_back(IR::AddressOf::create(baseName, node->symbol->unique_name));
    } else {
      // local baseName = unique_name
      ir.push_back(IR::Assign::create(baseName, node->symbol->unique_name));
    }
    SymbolPtr sym = node->symbol;
    auto arrType = std::dynamic_pointer_cast<ArrayType>(sym->type);
    std::string idxTemp;  // 临时变量存放下标表达式的值
    for (std::vector<int>::size_type i = 0; i < node->dims.size(); i++) {
      std::string offsetTemp = new_temp();
      // offsetTemp = #1 initially 为连乘做准备
      ir.push_back( IR::LoadImm::create(offsetTemp, 1));

      auto dimExp = node->dims[i];
      int subArrSize = 1;
      // 0) calculate subarray size
      for (std::vector<int>::size_type j = i + 1; j < arrType->dims.size(); j++) {
        subArrSize *= arrType->dims[j];
      }
      // 1) translateExp( dimExp, idxTemp )
      idxTemp = new_temp();
      // idxTemp 存放的是下标表达式的值 = dimExp
      auto exp_ir = translateExp(dimExp, idxTemp);
      std::move(exp_ir.begin(), exp_ir.end(), std::back_inserter(ir));

      // 2) offsetTemp = offsetTemp + <subDimensionSize> * idxTemp
      // 子数组大小 * 4 * 偏移量 就是这个维度的偏移量
      ir.push_back(IR::Binary::create(offsetTemp, idxTemp, BinaryOp::Mul, IR::imm_str(subArrSize * 4)));
      ir.push_back(IR::Binary::create(baseName, baseName, BinaryOp::Add, offsetTemp));
    }
    // 2.2) 现在 offsetTemp 里是字节偏移 且已经加进去了
    // 做 load: "place = *( baseName + #offsetTemp )"
    if (!place.empty()) {
      ir.push_back(IR::Load::create(place, baseName));
    }
  }
// #warning Not implemented: IRTranslator::translateLVal
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
  auto temp = new_temp();
  auto exp_ir = translateExp(node->exp, temp);
  std::move(exp_ir.begin(), exp_ir.end(), std::back_inserter(ir));
  // 添加一元运算指令
  if (!place.empty()) {
    ir.push_back(IR::Unary::create(place, node->op, temp));
  }
// #warning Not implemented: IRTranslator::translateUnaryExp
  return ir;
}

IR::Code IRTranslator::translateIfStmt(AST::IfStmtPtr node) {
  IR::Code ir;
  // 翻译条件表达式
  // 翻译 if 语句体
  // 如果有 else 语句，则翻译 else 语句体

  return ir;
}

IR::Code IRTranslator::translateWhileStmt(AST::WhileStmtPtr node) {
  IR::Code ir;
  // 翻译条件表达式
  // 翻译 while 语句体

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

IR::Code IRTranslator::translateParam(AST::ParamPtr node) {
  IR::Code ir;
  return ir;
}

IR::Code IRTranslator::translateExpStmt(AST::ExpStmtPtr node) {
  IR::Code ir;
  return ir;
}

IR::Code IRTranslator::translatePrimaryExp(AST::PrimaryExpPtr node) {
  IR::Code ir;
  // 翻译括号内的表达式
  auto exp_ir = translateExp(node->exp);
  std::move(exp_ir.begin(), exp_ir.end(), std::back_inserter(ir));
  return ir;
}