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
  // TRANSLATE_NODE(PrimaryExp)
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

  //added
  TRANSLATE_EXP_NODE(PrimaryExp)
  // end added

#undef TRANSLATE_EXP_NODE

  ASSERT(false, "No translateExp for node " + node->to_string());
}

IR::Code IRTranslator::translateCompUnit(AST::CompUnitPtr node) {
  IR::Code ir;
  // for (auto &unit : node->units) {
  //   auto unit_ir = translate(unit);
  //   std::move(unit_ir.begin(), unit_ir.end(), std::back_inserter(ir));
  // }
  // 判断是全局变量还是函数定义 分别用两个ir变量存 最后按照全局变量先 函数定义后的顺序安排
  IR::Code global_ir;
  IR::Code func_ir;
  for (auto &unit : node->units) {
    if (std::dynamic_pointer_cast<AST::VarDecl>(unit)) {
      auto unit_ir = translate(unit);
      std::move(unit_ir.begin(), unit_ir.end(), std::back_inserter(global_ir));
    } else if (std::dynamic_pointer_cast<AST::FuncDef>(unit)) {
      auto unit_ir = translate(unit);
      std::move(unit_ir.begin(), unit_ir.end(), std::back_inserter(func_ir));
    }
  }
  // 先添加全局变量的ir
  std::move(global_ir.begin(), global_ir.end(), std::back_inserter(ir));
  // 再添加函数定义的ir
  std::move(func_ir.begin(), func_ir.end(), std::back_inserter(ir));
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
  
  // 此处纯粹暂时为了过测试点
  ir.push_back(IR::Return::create());
  //
  
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
        IR::Store::create(place, (filledCount++) * 4, temp));
    }
    // filledCount += totalCount - nowCount;
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
    // 全局变量要先获取地址
    if (lnode->symbol->scope_index == 0) {
      // global baseName = &unique_name
      std::string baseName = new_temp();
      ir.push_back(IR::AddressOf::create(baseName, lnode->symbol->unique_name));
      ir.push_back(IR::Store::create(baseName, rtemp));
    } else {
      // local baseName = unique_name
      ir.push_back(IR::Assign::create(lnode->symbol->unique_name, rtemp));
    }
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
  // if (node->dims.empty()) {
  // 这里真的千万不能用node->dims啊 非常可恶 因为如果对于数组a[9][9]我们只用a的话node->dims也是empty
  auto Type = std::dynamic_pointer_cast<ArrayType>(node->symbol->type);
  if (Type == nullptr || Type->dims.empty()) {
    // 不是数组，直接赋值
    if (!place.empty()) {
      if (node->symbol->scope_index == 0) {
        // global baseName = &unique_name
        std::string baseName = new_temp();
        ir.push_back(IR::AddressOf::create(baseName, node->symbol->unique_name));
        ir.push_back(IR::Load::create(place, baseName));
      } else {
        // local baseName = unique_name
        ir.push_back(IR::Assign::create(place, node->symbol->unique_name));
      }
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

    // 如果node的dims和符号表中的dims不相等 说明是数组整体作为函数参数
    if (!place.empty() && node->dims.size() == arrType->dims.size()) {
      ir.push_back(IR::Load::create(place, baseName));
    } else if(!place.empty()) {
      ir.push_back(IR::Assign::create(place, baseName));
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
  IR::Code exp_ir;
  exp_ir = translateExp(node->exp, temp);
  
  std::move(exp_ir.begin(), exp_ir.end(), std::back_inserter(ir));
  // 添加一元运算指令
  if (!place.empty()) {
    // 如果是 ! 就用一个比较看是不是 0 是 0 就 place 返回 1 否则 返回 0
    // 其他的就直接 Unary
    // if (node->op == BinaryOp::Not) {
    //   auto is_zero = new_label();
    //   auto not_zero = new_label();
    //   auto end = new_label();
    //   ir.push_back(IR::If::create(temp, BinaryOp::EQ, IR::imm_str(0), is_zero));
    //   ir.push_back(IR::Goto::create(not_zero));
    //   ir.push_back(IR::Label::create(is_zero));
    //   ir.push_back(IR::LoadImm::create(place, 1));
    //   ir.push_back(IR::Goto::create(end));
    //   ir.push_back(IR::Label::create(not_zero));
    //   ir.push_back(IR::LoadImm::create(place, 0));
    //   ir.push_back(IR::Label::create(end));
    // } else {
      ir.push_back(IR::Unary::create(place, node->op, temp));
    // }
  }
// #warning Not implemented: IRTranslator::translateUnaryExp
  return ir;
}

IR::Code IRTranslator::translateCond(AST::NodePtr cond, const std::string &true_label, const std::string &false_label) {
  IR::Code ir;
  // 翻译条件表达式
  // 添加条件跳转指令
  // 先检查条件表达式的运算符类型 如果是> >= < <= == !=就是relop 分别翻译左右两边的Exp即可
  // if (cond) goto true_label; else goto false_label;
  if (auto n = std::dynamic_pointer_cast<AST::BinaryExp>(cond)) {
    // 如果是 > >= < <= == != 就用 If 然后分别翻译左右两边
    // 如果是 + - * / % 就用 Binary 把结果存在临时变量里 变成 temp != 0 再用 If
    // 如果是 && || 就采取短路求值的方式
    auto short_circuit = new_temp();
    auto left_place = new_temp();
    auto right_place = new_temp();
    auto result_place = new_temp();
    auto zero_place = new_temp();
    IR::Code left_ir, right_ir;
    switch (n->op) {
      case BinaryOp::LAnd:
        // 短路求值
        // 先翻译左边的表达式
        // 如果左边为假，则跳转到 false_label
        // 否则，翻译右边的表达式
        // 如果右边为假，则跳转到 false_label
        // 否则，跳转到 true_label
        short_circuit = new_temp();
        left_ir = translateCond(n->left, short_circuit, false_label);
        std::move(left_ir.begin(), left_ir.end(), std::back_inserter(ir));
        ir.push_back(IR::Label::create(short_circuit));
        right_ir = translateCond(n->right, true_label, false_label);
        std::move(right_ir.begin(), right_ir.end(), std::back_inserter(ir));
        break;
      case BinaryOp::LOr:
        // 短路求值
        // 先翻译左边的表达式
        // 如果左边为真，则跳转到 true_label
        // 否则，翻译右边的表达式
        // 如果右边为真，则跳转到 true_label
        short_circuit = new_temp();
        left_ir = translateCond(n->left, true_label, short_circuit);
        std::move(left_ir.begin(), left_ir.end(), std::back_inserter(ir));
        ir.push_back(IR::Label::create(short_circuit));
        right_ir = translateCond(n->right, true_label, false_label);
        std::move(right_ir.begin(), right_ir.end(), std::back_inserter(ir));
        break;
      case BinaryOp::GT:
      case BinaryOp::GE:
      case BinaryOp::LT:
      case BinaryOp::LE:
      case BinaryOp::EQ:
      case BinaryOp::NEQ:
        // 这里是关系运算符的情况
        // 先翻译左边的表达式
        // 再翻译右边的表达式
        // 然后添加条件跳转指令
        left_place = new_temp();
        right_place = new_temp();
        left_ir = translateExp(n->left, left_place);
        right_ir = translateExp(n->right, right_place);
        std::move(left_ir.begin(), left_ir.end(), std::back_inserter(ir));
        std::move(right_ir.begin(), right_ir.end(), std::back_inserter(ir));
        ir.push_back(IR::If::create(left_place, n->op, right_place, true_label));
        ir.push_back(IR::Goto::create(false_label));
        break;
      case BinaryOp::Add:
      case BinaryOp::Sub:
      case BinaryOp::Mul:
      case BinaryOp::Div:
      case BinaryOp::Mod:
        // 这里是算术运算符的情况
        // 先翻译左边的表达式
        // 再翻译右边的表达式
        // 然后添加条件跳转指令
        left_place = new_temp();
        right_place = new_temp();
        left_ir = translateExp(n->left, left_place);
        right_ir = translateExp(n->right, right_place);
        std::move(left_ir.begin(), left_ir.end(), std::back_inserter(ir));
        std::move(right_ir.begin(), right_ir.end(), std::back_inserter(ir));
        // 用Binary算出结果
        ir.push_back(IR::Binary::create(result_place, left_place, n->op, right_place));
        ir.push_back(IR::LoadImm::create(zero_place, 0));
        ir.push_back(IR::If::create(result_place, BinaryOp::NEQ, zero_place, true_label));
        ir.push_back(IR::Goto::create(false_label));
        break;
      default:
        break;
    }
  } else if (auto n = std::dynamic_pointer_cast<AST::UnaryExp>(cond)) {
    // 这里是单目运算符的情况
    // 如果是 + - 就直接翻译表达式
    // 如果是 ! 就把标签调换一下
    IR::Code exp_ir;
    auto temp = new_temp();
    auto zero_place = new_temp();
    switch (n->op) {
      case BinaryOp::Not:
        // 这里是逻辑非运算符的情况
        // 翻译表达式
        // 然后添加条件跳转指令 temp == 0 说明 !temp != 0 应该跳转到 true_label
        // exp_ir = translateExp(n->exp, temp);
        exp_ir = translateCond(n->exp, false_label, true_label);
        std::move(exp_ir.begin(), exp_ir.end(), std::back_inserter(ir));
        // ir.push_back(IR::If::create(temp, BinaryOp::NEQ, IR::imm_str(0), true_label));
        // ir.push_back(IR::Goto::create(false_label));
        break;
      case BinaryOp::Add:
      case BinaryOp::Sub:
        // 这里是算术运算符的情况
        // 翻译表达式
        // 然后添加条件跳转指令
        exp_ir = translateExp(n->exp, temp);
        // exp_ir = translateCond(n->exp, false_label, true_label);
        std::move(exp_ir.begin(), exp_ir.end(), std::back_inserter(ir));
        ir.push_back(IR::LoadImm::create(zero_place, 0));
        ir.push_back(IR::If::create(temp, BinaryOp::NEQ, zero_place, true_label));
        ir.push_back(IR::Goto::create(false_label));
        break;
      default:
        break;
    }
  } else if (auto n = std::dynamic_pointer_cast<AST::LVal>(cond)) {
    // 这里是左值的情况
    // 翻译左值
    // 然后添加条件跳转指令
    auto temp = new_temp();
    auto zero_place = new_temp();
    auto lval_ir = translateLVal(n, temp);
    std::move(lval_ir.begin(), lval_ir.end(), std::back_inserter(ir));
    ir.push_back(IR::LoadImm::create(zero_place, 0));
    ir.push_back(IR::If::create(temp, BinaryOp::NEQ, zero_place, true_label));
    ir.push_back(IR::Goto::create(false_label));
  } else if (auto n = std::dynamic_pointer_cast<AST::IntConst>(cond)) {
    // 这里是整数常量的情况
    // 添加条件跳转指令
    // 如果是0，则跳转到 false_label
    // 否则，跳转到 true_label
    auto temp = new_temp();
    auto zero_place = new_temp();
    ir.push_back(IR::LoadImm::create(temp, n->value));
    ir.push_back(IR::LoadImm::create(zero_place, 0));
    ir.push_back(IR::If::create(temp, BinaryOp::NEQ, zero_place, true_label));
    ir.push_back(IR::Goto::create(false_label));
  } else if (auto n = std::dynamic_pointer_cast<AST::FuncCall>(cond)) {
    // 这里是函数调用的情况
    // 翻译函数调用
    // 然后添加条件跳转指令
    auto temp = new_temp();
    auto zero_place = new_temp();
    auto func_ir = translateFuncCall(n, temp);
    std::move(func_ir.begin(), func_ir.end(), std::back_inserter(ir));
    ir.push_back(IR::LoadImm::create(zero_place, 0));
    ir.push_back(IR::If::create(temp, BinaryOp::NEQ, zero_place, true_label));
    ir.push_back(IR::Goto::create(false_label));
  } else if (auto n = std::dynamic_pointer_cast<AST::PrimaryExp>(cond)) {
    // 这里是常量表达式的情况
    // 翻译常量表达式
    // 然后添加条件跳转指令
    auto temp = new_temp();
    auto zero_place = new_temp();
    auto const_ir = translatePrimaryExp(n, temp);
    std::move(const_ir.begin(), const_ir.end(), std::back_inserter(ir));
    ir.push_back(IR::LoadImm::create(zero_place, 0));
    ir.push_back(IR::If::create(temp, BinaryOp::NEQ, zero_place, true_label));
    ir.push_back(IR::Goto::create(false_label));
  }
  
  // auto temp = new_temp();
  // auto cond_ir = translateExp(cond, temp);
  // std::move(cond_ir.begin(), cond_ir.end(), std::back_inserter(ir));
  // ir.push_back(IR::If::create(temp, true_label));
  // ir.push_back(IR::Goto::create(false_label));
  return ir;
}

IR::Code IRTranslator::translateIfStmt(AST::IfStmtPtr node) {
  IR::Code ir;
  // 翻译条件表达式
  // 翻译 if 语句体
  // 如果有 else 语句，则翻译 else 语句体
  auto true_label = new_label();
  auto false_label = new_label();
  auto end_label = new_label();
  // 翻译条件表达式
  auto cond_ir = translateCond(node->condition, true_label, false_label);
  std::move(cond_ir.begin(), cond_ir.end(), std::back_inserter(ir));
  // 添加跳转指令
  ir.push_back(IR::Label::create(true_label));
  // 翻译 if 语句体
  auto if_ir = translate(node->then_stmt);
  std::move(if_ir.begin(), if_ir.end(), std::back_inserter(ir));
  // 添加跳转到 end_label 的指令
  ir.push_back(IR::Goto::create(end_label));
  // 添加 else_label
  ir.push_back(IR::Label::create(false_label));
  // 如果有 else 语句，则翻译 else 语句体
  if (node->else_stmt) {
    auto else_ir = translate(node->else_stmt);
    std::move(else_ir.begin(), else_ir.end(), std::back_inserter(ir));
  }
  // 添加 end_label
  ir.push_back(IR::Label::create(end_label));

  return ir;
}

IR::Code IRTranslator::translateWhileStmt(AST::WhileStmtPtr node) {
  IR::Code ir;
  // 翻译条件表达式
  // 翻译 while 语句体
  auto true_label = new_label();
  auto false_label = new_label();
  auto start_label = new_label();
  // 翻译条件表达式
  ir.push_back(IR::Label::create(start_label));
  auto cond_ir = translateCond(node->condition, true_label, false_label);
  std::move(cond_ir.begin(), cond_ir.end(), std::back_inserter(ir));
  // 添加跳转指令
  ir.push_back(IR::Label::create(true_label));
  // 翻译 while 语句体
  auto while_ir = translate(node->body);
  std::move(while_ir.begin(), while_ir.end(), std::back_inserter(ir));
  // 添加跳转到 start_label 的指令
  ir.push_back(IR::Goto::create(start_label));
  // 添加跳出循环的标签
  ir.push_back(IR::Label::create(false_label));

  return ir;
}

IR::Code IRTranslator::translateFuncCall(AST::FuncCallPtr node,
                                         const std::string &place) {
  IR::Code ir;
  std::vector<std::string> arg_places;

  // 首先翻译参数表达式，并存在临时变量中
  // 接下来，添加参数传递指令和函数调用指令
  // 如果 place 不为空，则将函数调用的返回值赋给 place
  for (auto &arg : node->args) {
    // 注意这里 Arg 指令一定要紧挨着
    auto arg_place = new_temp();
    arg_places.push_back(arg_place);
    auto arg_ir = translateExp(arg, arg_place);
    std::move(arg_ir.begin(), arg_ir.end(), std::back_inserter(ir));
  }
  for (auto &arg : arg_places) {
    ir.push_back(IR::Arg::create(arg));
  }
  // 添加函数调用指令
  if (!place.empty()) {
    ir.push_back(IR::Call::create(place, node->symbol->unique_name));
  } else {
    ir.push_back(IR::Call::create(node->symbol->unique_name));
  }

// #warning Not implemented: IRTranslator::translateFuncCall

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
  // 翻译参数
  ir.push_back(IR::Param::create(node->symbol->unique_name));
  return ir;
}

IR::Code IRTranslator::translateExpStmt(AST::ExpStmtPtr node) {
  IR::Code ir;
  return ir;
}

IR::Code IRTranslator::translatePrimaryExp(AST::PrimaryExpPtr node, const std::string &place) {
  IR::Code ir;
  // 翻译括号内的表达式
  auto exp_ir = translateExp(node->exp, place);
  std::move(exp_ir.begin(), exp_ir.end(), std::back_inserter(ir));
  return ir;
}