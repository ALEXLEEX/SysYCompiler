#include "type_checker.hpp"

#include "common.hpp"

// for convenience, define some macros or helper 
static inline bool is_int(const TypePtr &t) {
  auto prim = std::dynamic_pointer_cast<PrimitiveType>(t);
  return (prim && prim->basic_type == BasicType::Int);
}

static inline bool is_void(const TypePtr &t) {
  auto prim = std::dynamic_pointer_cast<PrimitiveType>(t);
  return (prim && prim->basic_type == BasicType::Void);
}

TypeChecker::TypeChecker() {
  // 你需要在这里对 symbol_table 进行初始化
  // 插入一些内置函数，如 read 和 write
  symbol_table.enter_scope();
  // 插入内置函数 read 和 write
  symbol_table.add_symbol("read", FuncType::create(PrimitiveType::Int, {}));
  symbol_table.add_symbol("write", FuncType::create(PrimitiveType::Void, {PrimitiveType::Int}));
// #warning Not implemented: TypeChecker::TypeChecker
}

// 不同类型的节点调用对应的check函数 相当于重载
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
  // added
  CHECK_NODE(IfStmt)
  CHECK_NODE(WhileStmt)
  CHECK_NODE(ExpStmt)
  CHECK_NODE(PrimaryExp)
  //
  CHECK_NODE(LVal)
  CHECK_NODE(IntConst)
  CHECK_NODE(FuncCall)
  CHECK_NODE(UnaryExp)
  CHECK_NODE(BinaryExp)

// #warning Add more AST node types if needed

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
  BasicType bty = node->return_btype;
  // 思路：先创建一个FuncType对象，然后设置返回值类型和参数类型
  // 最后将FuncType对象插入符号表，并将符号表中的 symbol 挂到 FuncDef 节点上
  auto funcType = std::make_shared<FuncType>();

  // funcType->return_type = PrimitiveType::create(bty);
  // 设置函数返回值类型
  // 利用 PrimitiveType 已经创建好的静态成员变量
  // 可以直接使用 PrimitiveType::Int 或 PrimitiveType::Void
  if (bty == BasicType::Int) {
    funcType->return_type = PrimitiveType::Int;
  } else if (bty == BasicType::Void) {
    funcType->return_type = PrimitiveType::Void;
  } else {
    std::cerr << "Error: unknown return type at line " << node->lineno << std::endl;
    exit(1);
  }

  // 不需要再单独进入ParamPtr进行检查 直接在这里检查完
  // 所以没有实现checkParam
  std::vector<TypePtr> param_types;
  for (auto &param : node->params) {
    // 如果是整数类型 直接加入
    // 如果是数组类型 需要加入数组类型
    if (param->btype == BasicType::Int && param->dims.empty()) {
      param_types.push_back(PrimitiveType::Int);
    } else if (param->btype == BasicType::Int && !param->dims.empty()) {
      // 数组类型
      param_types.push_back(ArrayType::create(PrimitiveType::Int, param->dims));
    } else {
      std::cerr << "Error: unknown param type at line " << node->lineno << std::endl;
      exit(1);
    }
  }
  funcType->param_types = param_types;

  // insert symbol
  auto sym = symbol_table.add_symbol(node->name, funcType);
  if (!sym) {
    // redefined
    std::cerr << "Error: function " << node->name << " redefined at line " 
      << node->lineno << std::endl;
    exit(1);
  }

  // 将符号表中的 symbol 挂到 FuncDef 节点上
  node->symbol = sym;

  // set current function return type
  current_func_return_type = funcType->return_type;

  // enter scope
  // 相当于进入函数作用域 参数也是函数的内部变量 所以参数也要加入到新的scope中
  symbol_table.enter_scope();

  // insert params
  for (auto &param : node->params) {
    // 如果是数组类型 需要加入数组类型
    if (param->btype == BasicType::Int && param->dims.empty()) {
      auto sym = symbol_table.add_symbol(param->ident, PrimitiveType::Int);
      param->symbol = sym;
    } else if (param->btype == BasicType::Int && !param->dims.empty()) {
      // 数组类型
      auto sym = symbol_table.add_symbol(param->ident, ArrayType::create(PrimitiveType::Int, param->dims));
      param->symbol = sym;
    } else {
      std::cerr << "Error: unknown param type at line " << node->lineno << std::endl;
      exit(1);
    }
  }

  // check block
  // 因为要把参数加入到新的scope中
  // 在FuncDef就已经enter_scope了
  checkBlock(node->block, false);

  // exit scope
  symbol_table.exit_scope();

  // revert current function return type
  current_func_return_type = nullptr;
// #warning Not implemented: TypeChecker::checkFuncDef
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
  // auto type = PrimitiveType::create(var_type);
  // 判断变量是否已经被定义过
  // 如果有初始化表达式，你需要检查初始化表达式的类型是否和变量类型相同
  // 如果是数组，你还需要检查初始化表达式和数组的维度是否匹配，是否有溢出的情况

  // 思路：先创建一个Type对象
  // 然后根据变量类型设置Type对象的element_type和dims
  // 最后将Type对象插入符号表，并将符号表中的 symbol 挂到 VarDef 节点上
  // 如果有初始化表达式，还需要检查初始化表达式的类型是否和变量类型相同
  TypePtr type;
  if (node->dims.empty()) {
    type = PrimitiveType::create(var_type);
  } else {
    type = ArrayType::create(PrimitiveType::create(var_type), node->dims);
  }

  // 如果有初始化表达式
  if (node->initVal) {
    TypePtr init_type = checkInitVal(node->initVal, type);
    // 判断初始化表达式的类型是否和变量类型相同
    if (!type->equals(init_type)) {
      std::cerr << "Error: type mismatch at line " << node->lineno << std::endl;
      exit(1);
    }
  }

  // insert symbol
  auto sym = symbol_table.add_symbol(node->ident, type);
  if (!sym) {
    // redefined
    std::cerr << "Error: variable " << node->ident << " redefined at line " 
      << node->lineno << std::endl;
    exit(1);
  }

  // 将符号表中的 symbol 挂到 VarDef 节点上
  node->symbol = sym;
// #warning Not implemented: TypeChecker::checkVarDef
  return nullptr;
}

void TypeChecker::doCheckArrayInit(const std::vector<AST::InitValPtr> & sublist, const std::shared_ptr<ArrayType> & arrType, int lineno) {
    // total = product of arrType->dims 待初始化的元素总数
    int totalCount = 1;
    for (auto d : arrType->dims) totalCount *= d;
    
    int filledCount = 0; 
    // iterate over sublist { initVal, InitVal, ... }
    for (auto child : sublist) {
      auto initv = std::dynamic_pointer_cast<AST::InitVal>(child);
      if (initv->kind == AST::InitVal::Kind::EXP) {
        // { InitVal, Exp, InitVal, ... }
        //             ^ 到这里了
        // scalar => fill 1 element
        TypePtr t = check(initv->expr);
        // must match arrType->element_type if it's 1-d array,
        // or if arrType->dims.size>1 => means you are filling sub array?
        // here you see, to do real c-like we need a more advanced approach
        // We'll do a simplified version: if arrType->dims.size()==1 => must be int
        // else error or do partial approach
        if (! is_int(t)) {
          std::cerr << "Error: array init scalar not int at line " << lineno << std::endl;
          exit(1);
        }
        filledCount++;
        if (filledCount > totalCount) {
          std::cerr << "Error: too many init at line " << lineno << std::endl;
          exit(1);
        }
      } else {
        //  { InitVal, { ... } , ... } => we can interpret as sub array
        // dimension - 1
        if (arrType->dims.size() < 2) {
          // means we can't go deeper
          std::cerr << "Error: nested { } but no deeper array dimension left" << std::endl;
          exit(1);
        }
        // 首先应该用已填充元素数去整除 dims 的维度 找到最大的可初始化数组大小
        // 例如: int a[2][3][4] = { { {1,2,3,4}, {5,6,7,8} } }
        // 假如已经处理完 {1,2,3,4} 下一个到 {5,6,7,8} 那么 filledCount = 4
        // 现在用 filledCount = 4 去整除 dims[2] = 4 
        // filledCount % dims[2] == 0 but filledCount % (dims[1] * dims[2]) != 0
        // 所以我们要新创建一个数组类型 subArr[4] 去处理 {5,6,7,8}
        int i = 0;
        int temp = 1;
        
        for (i = arrType->dims.size()-1; i >= 1; i--) {
          temp *= arrType->dims[i];
          // std::cerr << "temp = " << temp << " i = " << i << std::endl;
          // std::cerr << "filledCount = " << filledCount << std::endl;
          if (filledCount % temp != 0) {
            break;
          }
        }
        i++;

        // build a sub array type
        auto subArr = std::make_shared<ArrayType>();
        subArr->element_type = arrType->element_type;
        
        // subArr->dims.assign(arrType->dims.begin()+1, arrType->dims.end());
        for (std::vector<int>::size_type j = i; j < arrType->dims.size(); j++) {
          subArr->dims.push_back(arrType->dims[j]);
        }

        int subTotal = 1;
        for (auto d : subArr->dims) subTotal *= d;

        // check sublist recursively
        // fill up to subTotal elements
        // we can do something like:
        auto & subSubs = initv->subInitVals;
        doCheckArrayInit(subSubs, subArr, lineno);

        // that function might fill subTotal if subSubs not enough => implicit 0
        filledCount += subTotal; 
        if (filledCount > totalCount) {
          std::cerr << "Error: too many init in bigger dimension, line " << lineno << std::endl;
          exit(1);
        }
      }
    }
    // if filledCount < totalCount => implicit fill 0
    // if (filledCount < totalCount) {
    //   std::cerr << "Warning: implicit 0 fill at line " << lineno << std::endl;
    // }
    return;
}

TypePtr TypeChecker::checkInitVal(AST::InitValPtr node, const TypePtr &target_type) {
  // 思路：需要判断初始化表达式的类型是否和变量类型相同
  // 如果是数组，还需要检查初始化表达式和数组的维度是否匹配，是否有溢出的情况
  // 思路：先判断 target_type 是否是什么类型
  // 如果是 EXP 就 check Exp 类型即可
  if (node->kind == AST::InitVal::Kind::EXP) {
    TypePtr exprT = check(node->expr);  // expr => checkExp
    // 对于整型变量 => 必须是整型表达式
    if (!exprT->equals(target_type)) {
        std::cerr << "Error: initVal type mismatch at line " << node->lineno << std::endl;
        exit(1);
    }
    // 返回 target_type, 仅表示 initVal 是单纯 int 类型
    return target_type;
  } else {
    // kind == Kind::BRCAE => { ... }
    // 如果 target_type是 array => 继续深层次初始化
    // 如果 target_type是 int => 不能用花括号初始化

    auto arrType = std::dynamic_pointer_cast<ArrayType>(target_type);
    if (!arrType) {
      // 直接报错 不应该出现对非数组但是用花括号初始化的情况
      std::cerr << "Error: initVal type {} mismatch sclar at line " << node->lineno << std::endl;
      exit(1);
    }
    // target_type is array => do c-like array initialization 
    // define a helper that does the actual recursion and indexing

    doCheckArrayInit(node->subInitVals, arrType, node->lineno);
    // 如果函数内部没有报错，说明初始化表达式和数组的维度是否匹配
    // 返回 target_type, 表示 initVal 是数组类型并且维度匹配
    return arrType;
  }
}


TypePtr TypeChecker::checkBlock(AST::BlockPtr node, bool new_scope) {
  // 检查块内的每个语句
  // 如果 new_scope 为 true
  // 你需要在进入和退出块时更新符号表，创建、销毁新的作用域
  // 思路：如果 new_scope 为 false
  // 你需要在当前作用域中继续检查
  if (new_scope) {
    symbol_table.enter_scope();
  }
  for (auto &stmt : node->stmts) {
    check(stmt);
  }
  if (new_scope) {
    symbol_table.exit_scope();
  }
// #warning Not implemented: TypeChecker::checkBlock
  return nullptr;
}

TypePtr TypeChecker::checkAssignStmt(AST::AssignStmtPtr node) {
  TypePtr lval_type = check(node->lval);
  TypePtr expr_type = check(node->exp);
  // 判断赋值号两边的类型是否相同
  // 我们实验中只支持 int 类型
  // 因此你需要判断 lval_type 和 expr_type 是否都为 int 类型
  if (!lval_type->equals(expr_type) || !is_int(lval_type) || !is_int(expr_type)) {
    std::cerr << "Error: assign type mismatch at line " << node->lineno << std::endl;
    exit(1);
  }
// #warning Not implemented: TypeChecker::checkAssignStmt
  return lval_type;
}

TypePtr TypeChecker::checkReturnStmt(AST::ReturnStmtPtr node) {
  // TypePtr expr_type = check(node->exp);
  // 判断返回值类型是否和函数声明的返回值类型相同
  // 思路：如果函数声明的返回值类型是 void，那么返回值应该为空
  // 如果函数声明的返回值类型是 int，那么返回值应该是 int
  if (node->exp == nullptr) {
    if (!is_void(current_func_return_type)) {
      std::cerr << "Error: return type mismatch at line " << node->lineno << std::endl;
      exit(1);
    }
  } else {
    TypePtr expr_type = check(node->exp);
    if (!current_func_return_type->equals(expr_type)) {
      std::cerr << "Error: return type mismatch at line " << node->lineno << std::endl;
      exit(1);
    }
  }
// #warning Not implemented: TypeChecker::checkReturnStmt
  return nullptr;
}

// added
TypePtr TypeChecker::checkIfStmt(AST::IfStmtPtr node) {
  // 控制流语句的条件表达式必须是整形表达式
  TypePtr cond_type = check(node->condition);
  if (!is_int(cond_type)) {
    std::cerr << "Error: if condition type mismatch at line " << node->lineno << std::endl;
    exit(1);
  }
  check(node->then_stmt);
  if (node->else_stmt) {
    check(node->else_stmt);
  }
  return nullptr;
}

TypePtr TypeChecker::checkWhileStmt(AST::WhileStmtPtr node) {
  // 控制流语句的条件表达式必须是整形表达式
  TypePtr cond_type = check(node->condition);
  if (!is_int(cond_type)) {
    std::cerr << "Error: while condition type mismatch at line " << node->lineno << std::endl;
    exit(1);
  }
  check(node->body);
  return nullptr;
}

TypePtr TypeChecker::checkExpStmt(AST::ExpStmtPtr node) {
  if (node->exp) {
    check(node->exp);
  }
  return nullptr;
}

TypePtr TypeChecker::checkPrimaryExp(AST::PrimaryExpPtr node) {
  return check(node->exp);
}
// end added

TypePtr TypeChecker::checkLVal(AST::LValPtr node) {
  // 你需要在这里查找符号表，判断变量是否被定义过
  // 根据符号表中的信息设置 LVal 的类型
  // 若变量未定义，你需要报错
  // 否则，将符号表中的 symbol 挂到 LVal 节点上
  // 如果 LVal 是数组，你还需要根据下标索引来设置 LVal 的类型

  // 思路：先查找符号表，判断变量是否被定义过
  // 如果变量未定义，报错
  // 否则，根据符号表中的信息设置 LVal 的类型
  // 如果 LVal 是数组，根据下标索引来设置 LVal 的类型
  auto sym = symbol_table.find_symbol(node->ident, false);
  if (!sym) {
    std::cerr << "Error: undefined variable " << node->ident << " not defined at line " 
      << node->lineno << std::endl;
    exit(1);
  }
  node->symbol = sym;
  // 如果 LVal 是数组，根据下标索引来设置 LVal 的类型
  if (node->dims.empty()) {
    // 不是数组直接返回整数类型
    return sym->type;
  }
  // otherwise we expect sym->type is array
  auto arrT = std::dynamic_pointer_cast<ArrayType>(sym->type);
  if (!arrT) {
    std::cerr << "Error: " << node->ident << " is not an array at line "
      << node->lineno << std::endl;
    exit(1);
  }
  // check the number of subscript vs arrT->dims
  if (node->dims.size() > arrT->dims.size()) {
    std::cerr << "Error: array subscript dimension exceed at line "
      << node->lineno << std::endl;
    exit(1);
  }
  // all subscript must be int
  for (auto &dimExp : node->dims) {
    auto dt = check(dimExp);
    if (!is_int(dt)) {
      std::cerr << "Error: array subscript not int at line "
        << node->lineno << std::endl;
      exit(1);
    }
  }
  // final type => if subscript == dims.size, it's an int
  // otherwise it's a smaller dimension array
  size_t remain = arrT->dims.size() - node->dims.size();
  if (remain == 0) {
    // int
    return PrimitiveType::Int;
  } else {
    // build a new ArrayType with remain dims
    auto newArr = std::make_shared<ArrayType>();
    newArr->element_type = arrT->element_type;
    newArr->dims.assign(arrT->dims.begin() + node->dims.size(), arrT->dims.end());
    return newArr;
  }
// #warning Not implemented: TypeChecker::checkLVal
  // 你需要返回 LVal 的类型
  // return nullptr;
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

  // find symbol
  auto sym = symbol_table.find_symbol(node->name, false);
  if (!sym) {
    std::cerr << "Error: call undefined function " 
      << node->name << " at line " << node->lineno << std::endl;
    exit(1);
  }
  node->symbol = sym;

  // must be function
  auto ftype = std::dynamic_pointer_cast<FuncType>(sym->type);
  if (!ftype) {
    std::cerr << "Error: " << node->name << " is not a function at line "
      << node->lineno << std::endl;
    exit(1);
  }

  // check param count
  if (ftype->param_types.size() != node->args.size()) {
    std::cerr << "Error: function param count mismatch for " 
      << node->name << " at line " << node->lineno << std::endl;
    exit(1);
  }
  // check each param type
  for (size_t i=0; i<node->args.size(); i++) {
    auto argT = check(node->args[i]);
    auto paramT = ftype->param_types[i];
    if (!argT->equals(paramT)) {
      std::cerr << "Error: function param type mismatch for " 
        << node->name << " param index " << i 
        << " at line " << node->lineno << std::endl;
      exit(1);
    }
  }
  // function call expression type is the function's return type
  return ftype->return_type;
// #warning Not implemented: TypeChecker::checkFuncCall
  // 你需要返回函数调用表达式的类型
  // return nullptr;
}

TypePtr TypeChecker::checkUnaryExp(AST::UnaryExpPtr node) {
  auto type = check(node->exp);
  // 一元表达式只支持 int 类型，因此你需要判断 type 是否为 int
  if (!is_int(type)) {
    std::cerr << "Error: unary operator expects int at line " 
      << node->lineno << std::endl;
    exit(1);
  }
// #warning Not implemented: TypeChecker::checkUnaryExp
  return PrimitiveType::Int;
}

TypePtr TypeChecker::checkBinaryExp(AST::BinaryExpPtr node) {
  TypePtr left_type = check(node->left);
  TypePtr right_type = check(node->right);
  // 二元表达式只支持 int 类型，因此你需要判断左右表达式的类型是否为 int
  if (!is_int(left_type) || !is_int(right_type)) {
    std::cerr << "Error: binary op expects int at line "
      << node->lineno << std::endl;
    exit(1);
  }
// #warning Not implemented: TypeChecker::checkBinaryExp
  return PrimitiveType::Int;
}
