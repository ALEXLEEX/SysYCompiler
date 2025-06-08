/**
 * SysY语言语法分析器实现
 * 实现递归下降语法分析器，将Token流转换为抽象语法树(AST)
 */
#include "parser.hpp"

#include <stdexcept>
using namespace AST;

/**
 * 构造函数：初始化语法分析器并读取第一个Token
 * @param l 词法分析器引用
 */
Parser::Parser(Lexer &l) : lex(l) { advance(); }

/**
 * 前进到下一个Token
 */
void Parser::advance() { tok = lex.next(); }

/**
 * 检查当前Token是否为指定类型
 * @param t 要检查的Token类型
 * @return 是否匹配
 */
bool Parser::match(TokenType t) { return tok.type == t; }

/**
 * 消费一个指定类型的Token
 * @param t 期望的Token类型
 * @return 被消费的Token
 * @throws std::runtime_error 如果当前Token类型不匹配
 */
Token Parser::consume(TokenType t) {
  if (tok.type != t) {
    throw std::runtime_error("Unexpected token");
  }
  Token cur = tok;
  advance();
  return cur;
}

/**
 * 开始解析，返回编译单元的AST
 * @return 编译单元AST节点
 */
NodePtr Parser::parse() { return parseCompUnit(); }

/**
 * 解析编译单元 (CompUnit)
 * CompUnit -> (Decl | FuncDef)*
 * 编译单元是整个程序的顶层结构，包含全局变量声明和函数定义
 */
CompUnitPtr Parser::parseCompUnit() {
  auto cu = std::make_shared<CompUnit>(nullptr);
  cu->units.clear();

  // 循环解析直到文件结束
  while (!match(TokenType::END)) {
    // 检查是否为类型关键字（int或void）
    if (match(TokenType::INT) || match(TokenType::VOID)) {
      TokenType t = tok.type;
      advance();                                // 保存类型并前进
      Token idTok = consume(TokenType::IDENT);  // 消费标识符

      if (match(TokenType::LPAREN)) {
        // 函数定义：类型 标识符 (参数列表) 语句块
        advance();  // 消费 '('
        std::vector<ParamPtr> params;
        if (!match(TokenType::RPAREN)) {
          params = parseFuncFParams();  // 解析参数列表
        }
        consume(TokenType::RPAREN);  // 消费 ')'
        auto block = parseBlock();   // 解析函数体

        // 创建函数定义AST节点
        BasicType ret =
            (t == TokenType::INT) ? BasicType::Int : BasicType::Void;
        auto f =
            std::make_shared<FuncDef>(ret, idTok.str.c_str(), block, params);
        cu->add_unit(f);
      } else {
        // 变量声明：类型 变量定义列表 ;
        // 此时已经读取了类型和第一个标识符，需要手动构建变量声明
        std::vector<VarDefPtr> defs;

        // 解析第一个变量定义的剩余部分（数组维度和初始值）
        std::vector<int> dims;  // 数组维度
        while (match(TokenType::LBRACK)) {
          advance();                                 // 消费 '['
          auto eTok = consume(TokenType::INTCONST);  // 获取数组大小
          dims.push_back(eTok.value);
          consume(TokenType::RBRACK);  // 消费 ']'
        }

        // 检查是否有初始值
        InitValPtr init = nullptr;
        if (match(TokenType::ASSIGN)) {
          advance();              // 消费 '='
          init = parseInitVal();  // 解析初始值
        }

        // 创建第一个变量定义
        auto var = std::make_shared<VarDef>(idTok.str.c_str());
        for (int d : dims) {
          var->add_dim(d);
        }
        if (init) {
          var->add_initVal(init);
        }
        defs.push_back(var);

        // 解析后续的变量定义（用逗号分隔）
        while (match(TokenType::COMMA)) {
          advance();                      // 消费 ','
          defs.push_back(parseVarDef());  // 解析下一个变量定义
        }
        consume(TokenType::SEMICOLON);  // 消费 ';'

        // 创建变量声明AST节点
        auto vd = std::make_shared<VarDecl>(defs.front());
        for (size_t i = 1; i < defs.size(); ++i) {
          vd->add_def(defs[i]);
        }
        vd->btype = BasicType::Int;  // 目前只支持int类型
        cu->add_unit(vd);
      }
    } else {
      throw std::runtime_error("Unexpected token in compilation unit");
    }
  }
  return cu;
}

/**
 * 解析变量定义 (VarDef)
 * VarDef -> Ident {'[' ConstExp ']'} ['=' InitVal]
 * 解析单个变量的定义，包括变量名、数组维度和可选的初始值
 */
VarDefPtr Parser::parseVarDef() {
  Token idTok = consume(TokenType::IDENT);  // 获取变量名
  std::vector<int> dims;                    // 存储数组维度

  // 解析数组维度 [size1][size2]...
  while (match(TokenType::LBRACK)) {
    advance();                                 // 消费 '['
    Token val = consume(TokenType::INTCONST);  // 获取数组大小
    consume(TokenType::RBRACK);                // 消费 ']'
    dims.push_back(val.value);
  }

  // 解析可选的初始值
  InitValPtr init = nullptr;
  if (match(TokenType::ASSIGN)) {
    advance();              // 消费 '='
    init = parseInitVal();  // 解析初始值
  }

  // 创建变量定义AST节点
  auto var = std::make_shared<VarDef>(idTok.str.c_str());
  for (int d : dims) {
    var->add_dim(d);  // 添加数组维度
  }
  if (init) {
    var->add_initVal(init);  // 添加初始值（如果有）
  }
  return var;
}

/**
 * 解析数组维度的辅助函数
 * @param dims 存储维度大小的向量引用
 */
void Parser::parseArrayDims(std::vector<int> &dims) {
  while (match(TokenType::LBRACK)) {
    advance();                                 // 消费 '['
    Token val = consume(TokenType::INTCONST);  // 获取数组大小
    consume(TokenType::RBRACK);                // 消费 ']'
    dims.push_back(val.value);
  }
}

/**
 * 解析初始值 (InitVal)
 * InitVal -> Exp | '{' [InitVal {',' InitVal}] '}'
 * 处理变量和数组的初始值，支持单个表达式或初始值列表
 */
InitValPtr Parser::parseInitVal() {
  if (match(TokenType::LBRACE)) {
    // 数组初始值列表：{ val1, val2, ... }
    advance();  // 消费 '{'

    if (match(TokenType::RBRACE)) {
      // 空的初始值列表 {}
      advance();
      return std::make_shared<InitVal>();
    }

    // 解析第一个初始值
    auto first = parseInitVal();
    std::vector<InitValPtr> vals;
    vals.push_back(first);

    // 解析后续的初始值（用逗号分隔）
    while (match(TokenType::COMMA)) {
      advance();                       // 消费 ','
      vals.push_back(parseInitVal());  // 递归解析下一个初始值
    }
    consume(TokenType::RBRACE);  // 消费 '}'

    // 创建初始值列表AST节点
    auto list = std::make_shared<InitVal>(vals.front());
    for (size_t i = 1; i < vals.size(); ++i) {
      list->add_sub(vals[i]);
    }
    return list;
  } else {
    // 单个表达式作为初始值
    auto exp = parseExp();
    return std::make_shared<InitVal>(exp);
  }
}

/**
 * 解析函数定义 (FuncDef)
 * 注意：在新的parseCompUnit实现中未使用此函数
 */
FuncDefPtr Parser::parseFuncDef() {
  // not used in new parseCompUnit
  return nullptr;
}

/**
 * 解析函数形参列表 (FuncFParams)
 * FuncFParams -> FuncFParam {',' FuncFParam}
 * 解析函数定义中的参数列表
 */
std::vector<ParamPtr> Parser::parseFuncFParams() {
  std::vector<ParamPtr> params;
  params.push_back(parseFuncFParam());  // 解析第一个参数

  // 解析后续参数（用逗号分隔）
  while (match(TokenType::COMMA)) {
    advance();                            // 消费 ','
    params.push_back(parseFuncFParam());  // 解析下一个参数
  }
  return params;
}

/**
 * 解析函数形参 (FuncFParam)
 * FuncFParam -> BType Ident ['[' ']' {'[' ConstExp ']'}]
 * 解析单个函数参数，支持数组参数
 */
ParamPtr Parser::parseFuncFParam() {
  consume(TokenType::INT);                  // 目前只支持int类型参数
  Token idTok = consume(TokenType::IDENT);  // 获取参数名
  std::vector<int> dims;                    // 存储数组维度

  // 检查是否为数组参数
  if (match(TokenType::LBRACK)) {
    advance();                   // 消费 '['
    consume(TokenType::RBRACK);  // 消费 ']'
    dims.push_back(-1);          // 第一维大小未知，用-1表示
    parseArrayDims(dims);        // 解析后续维度
  }

  // 创建参数AST节点
  auto p = std::make_shared<Param>(idTok.str.c_str());
  for (size_t i = 0; i < dims.size(); ++i) {
    if (i == 0 && dims[i] == -1) {
      p->add_dim(-1);  // 第一维未知
    } else {
      p->add_dim(dims[i]);  // 其他维度
    }
  }
  return p;
}

/**
 * 解析语句块 (Block)
 * Block -> '{' {BlockItem} '}'
 * 解析由大括号包围的语句块
 */
BlockPtr Parser::parseBlock() {
  consume(TokenType::LBRACE);  // 消费 '{'
  std::vector<NodePtr> stmts;  // 存储语句块中的所有语句

  // 解析语句块中的所有项目
  while (!match(TokenType::RBRACE)) {
    stmts.push_back(parseBlockItem());  // 解析语句块项
  }
  consume(TokenType::RBRACE);  // 消费 '}'

  // 创建语句块AST节点
  auto blk = std::make_shared<Block>();
  for (auto &s : stmts) {
    blk->add_stmt(s);
  }
  return blk;
}

/**
 * 解析语句块项 (BlockItem)
 * BlockItem -> Decl | Stmt
 * 语句块项可以是变量声明或语句
 */
NodePtr Parser::parseBlockItem() {
  if (match(TokenType::INT)) {
    // 变量声明：int varDef1, varDef2, ...;
    advance();                   // 消费 'int'
    auto first = parseVarDef();  // 解析第一个变量定义
    std::vector<VarDefPtr> defs;
    defs.push_back(first);

    // 解析后续变量定义（用逗号分隔）
    while (match(TokenType::COMMA)) {
      advance();  // 消费 ','
      defs.push_back(parseVarDef());
    }
    consume(TokenType::SEMICOLON);  // 消费 ';'

    // 创建变量声明AST节点
    auto vd = std::make_shared<VarDecl>(defs.front());
    for (size_t i = 1; i < defs.size(); ++i) {
      vd->add_def(defs[i]);
    }
    vd->btype = BasicType::Int;
    return vd;
  } else {
    // 其他语句
    return parseStmt();
  }
}

/**
 * 解析语句 (Stmt)
 * Stmt -> LVal '=' Exp ';'        // 赋值语句
 *      | [Exp] ';'                // 表达式语句或空语句
 *      | Block                    // 语句块
 *      | 'if' '(' Cond ')' Stmt ['else' Stmt]  // if语句
 *      | 'while' '(' Cond ')' Stmt             // while语句
 *      | 'return' [Exp] ';'                    // return语句
 */
NodePtr Parser::parseStmt() {
  // 语句块
  if (match(TokenType::LBRACE)) {
    return parseBlock();
  }

  // if语句
  if (match(TokenType::IF)) {
    int line = tok.lineno;       // 记录行号用于错误报告
    advance();                   // 消费 'if'
    consume(TokenType::LPAREN);  // 消费 '('
    auto cond = parseCond();     // 解析条件表达式
    consume(TokenType::RPAREN);  // 消费 ')'
    auto then_s = parseStmt();   // 解析then分支语句

    if (match(TokenType::ELSE)) {
      // 有else分支的if语句
      advance();                  // 消费 'else'
      auto else_s = parseStmt();  // 解析else分支语句
      auto node = std::make_shared<IfStmt>(cond, then_s, else_s);
      node->lineno = line;
      return node;
    } else {
      // 只有then分支的if语句
      auto node = std::make_shared<IfStmt>(cond, then_s);
      node->lineno = line;
      return node;
    }
  }

  // while语句
  if (match(TokenType::WHILE)) {
    int line = tok.lineno;       // 记录行号
    advance();                   // 消费 'while'
    consume(TokenType::LPAREN);  // 消费 '('
    auto cond = parseCond();     // 解析条件表达式
    consume(TokenType::RPAREN);  // 消费 ')'
    auto body = parseStmt();     // 解析循环体
    auto node = std::make_shared<WhileStmt>(cond, body);
    node->lineno = line;
    return node;
  }

  // return语句
  if (match(TokenType::RETURN)) {
    int line = tok.lineno;
    advance();  // 消费 'return'
    if (match(TokenType::SEMICOLON)) {
      // 无返回值的return语句
      advance();
      auto node = std::make_shared<ReturnStmt>();
      node->lineno = line;
      return node;
    }
    // 有返回值的return语句
    auto e = parseExp();
    consume(TokenType::SEMICOLON);
    auto node = std::make_shared<ReturnStmt>(e);
    node->lineno = line;
    return node;
  }

  // 空语句
  if (match(TokenType::SEMICOLON)) {
    int line = tok.lineno;
    advance();  // 消费 ';'
    auto node = std::make_shared<ExpStmt>();
    node->lineno = line;
    return node;
  }

  // 表达式语句或赋值语句
  NodePtr exp = parseExp();  // 先解析表达式
  if (match(TokenType::ASSIGN)) {
    // 赋值语句：左值 = 表达式
    auto lval = std::dynamic_pointer_cast<LVal>(exp);  // 将表达式转换为左值
    if (!lval) {
      throw std::runtime_error("assign lhs not lval");  // 赋值左边必须是左值
    }
    advance();  // 消费 '='
    int line = tok.lineno;
    auto e = parseExp();            // 解析右边的表达式
    consume(TokenType::SEMICOLON);  // 消费 ';'
    auto node = std::make_shared<AssignStmt>(lval, e);
    node->lineno = line;
    return node;
  } else {
    // 纯表达式语句
    consume(TokenType::SEMICOLON);  // 消费 ';'
    return exp;
  }
}

/**
 * 解析表达式 (Exp)
 * Exp -> AddExp
 * 表达式的入口，从最低优先级开始解析
 */
NodePtr Parser::parseExp() {
  return parseBinary(1);  // 从优先级1开始（最低优先级）
}

/**
 * 解析条件表达式 (Cond)
 * Cond -> LOrExp
 * 条件表达式用于if和while语句中
 */
NodePtr Parser::parseCond() {
  return parseBinary(1);  // 从优先级1开始
}

/**
 * 获取运算符的优先级
 * @param t Token类型
 * @return 优先级（数字越大优先级越高，0表示不是二元运算符）
 */
int Parser::precedence(TokenType t) {
  switch (t) {
    case TokenType::OR:
      return 1;  // || 逻辑或，优先级最低
    case TokenType::AND:
      return 2;  // && 逻辑与
    case TokenType::EQ:
    case TokenType::NEQ:
      return 3;  // == != 相等比较
    case TokenType::LT:
    case TokenType::GT:
    case TokenType::LE:
    case TokenType::GE:
      return 4;  // < > <= >= 关系比较
    case TokenType::ADD:
    case TokenType::SUB:
      return 5;  // + - 加减法
    case TokenType::MUL:
    case TokenType::DIV:
    case TokenType::MOD:
      return 6;  // * / % 乘除取模，优先级最高
    default:
      return 0;  // 不是二元运算符
  }
}

/**
 * 将Token类型转换为二元运算符类型
 * @param t Token类型
 * @return 对应的二元运算符枚举值
 */
BinaryOp Parser::toBinaryOp(TokenType t) {
  switch (t) {
    case TokenType::OR:
      return BinaryOp::LOr;  // 逻辑或
    case TokenType::AND:
      return BinaryOp::LAnd;  // 逻辑与
    case TokenType::EQ:
      return BinaryOp::EQ;  // 相等
    case TokenType::NEQ:
      return BinaryOp::NEQ;  // 不等
    case TokenType::LT:
      return BinaryOp::LT;  // 小于
    case TokenType::GT:
      return BinaryOp::GT;  // 大于
    case TokenType::LE:
      return BinaryOp::LE;  // 小于等于
    case TokenType::GE:
      return BinaryOp::GE;  // 大于等于
    case TokenType::ADD:
      return BinaryOp::Add;  // 加法
    case TokenType::SUB:
      return BinaryOp::Sub;  // 减法
    case TokenType::MUL:
      return BinaryOp::Mul;  // 乘法
    case TokenType::DIV:
      return BinaryOp::Div;  // 除法
    case TokenType::MOD:
      return BinaryOp::Mod;  // 取模
    case TokenType::NOT:
      return BinaryOp::Not;  // 逻辑非（一元运算符）
    default:
      return BinaryOp::Add;  // 默认值
  }
}

/**
 * 解析二元表达式（使用运算符优先级分析法）
 * 这是一个递归函数，实现了运算符优先级的正确处理
 * @param prec 当前处理的最低优先级
 * @return 表达式AST节点
 */
NodePtr Parser::parseBinary(int prec) {
  NodePtr left = parseUnary();  // 先解析左操作数（一元表达式）

  // 持续处理当前优先级及更高优先级的运算符
  while (true) {
    int p = precedence(tok.type);  // 获取当前运算符的优先级
    if (p < prec || p == 0) {
      break;  // 优先级不够或不是运算符，退出循环
    }

    TokenType op = tok.type;
    int line = tok.lineno;
    advance();  // 消费运算符

    // 递归解析右操作数，优先级+1确保左结合性
    NodePtr right = parseBinary(p + 1);

    // 创建二元表达式AST节点
    auto node = std::make_shared<BinaryExp>(toBinaryOp(op), left, right);
    node->lineno = line;
    left = node;  // 更新左操作数，为下一轮循环做准备
  }
  return left;
}

/**
 * 解析一元表达式 (UnaryExp)
 * UnaryExp -> PrimaryExp | Ident '(' [FuncRParams] ')' | UnaryOp UnaryExp
 * 处理一元运算符（+、-、!）和函数调用
 */
NodePtr Parser::parseUnary() {
  // 检查是否为一元运算符
  if (tok.type == TokenType::ADD || tok.type == TokenType::SUB ||
      tok.type == TokenType::NOT) {
    TokenType op = tok.type;
    int line = tok.lineno;
    advance();                 // 消费一元运算符
    auto expr = parseUnary();  // 递归解析操作数（支持多个一元运算符连续使用）
    auto node = std::make_shared<UnaryExp>(toBinaryOp(op), expr);
    node->lineno = line;
    return node;
  }
  // 不是一元运算符，解析基本表达式
  return parsePrimary();
}

/**
 * 解析基本表达式 (PrimaryExp)
 * PrimaryExp -> '(' Exp ')' | LVal | Number | Ident '(' [FuncRParams] ')'
 * 处理括号表达式、整数常量、变量/数组引用和函数调用
 */
NodePtr Parser::parsePrimary() {
  // 括号表达式：(expression)
  if (match(TokenType::LPAREN)) {
    int line = tok.lineno;
    advance();                   // 消费 '('
    auto e = parseExp();         // 解析括号内的表达式
    consume(TokenType::RPAREN);  // 消费 ')'
    yylineno = line;             // 设置行号（可能用于调试）
    auto node = std::make_shared<PrimaryExp>(e);
    node->lineno = line;
    return node;
  }

  // 整数常量
  if (match(TokenType::INTCONST)) {
    int line = tok.lineno;
    int v = tok.value;  // 获取整数值
    advance();          // 消费整数常量
    yylineno = line;
    auto node = std::make_shared<IntConst>(v);
    node->lineno = line;
    return node;
  }

  // 标识符：可能是变量、数组元素或函数调用
  if (match(TokenType::IDENT)) {
    std::string name = tok.str;  // 获取标识符名称
    int line = tok.lineno;
    advance();  // 消费标识符

    if (match(TokenType::LPAREN)) {
      // 函数调用：functionName(arg1, arg2, ...)
      advance();  // 消费 '('
      auto call = std::make_shared<FuncCall>(name.c_str());

      // 解析参数列表（如果有）
      if (!match(TokenType::RPAREN)) {
        call->add_arg(parseExp());  // 解析第一个参数
        while (match(TokenType::COMMA)) {
          advance();                  // 消费 ','
          call->add_arg(parseExp());  // 解析下一个参数
        }
      }
      consume(TokenType::RPAREN);  // 消费 ')'
      call->lineno = line;
      return call;
    } else {
      // 变量或数组元素：varName 或 arrayName[index1][index2]...
      auto lval = std::make_shared<LVal>(name);

      // 解析数组下标（如果有）
      while (match(TokenType::LBRACK)) {
        advance();                   // 消费 '['
        auto idx = parseExp();       // 解析下标表达式
        consume(TokenType::RBRACK);  // 消费 ']'
        lval->add_dim(idx);
      }
      lval->lineno = line;
      return lval;
    }
  }

  // 如果都不匹配，抛出异常
  throw std::runtime_error("Unexpected token in primary");
}

/**
 * 总结：
 * 这个文件实现了SysY语言的完整递归下降语法分析器。
 *
 * 核心算法和技术：
 * 1. 递归下降分析：每个语法规则对应一个递归函数
 * 2. 运算符优先级分析：使用parseBinary函数处理表达式的优先级和结合性
 * 3. 向前看：利用词法分析器的peek功能进行语法决策
 * 4. AST构建：每个解析函数返回相应的AST节点
 *
 * 支持的语法特性：
 * 1. 变量声明和定义（包括数组）
 * 2. 函数定义和调用
 * 3. 各种语句：if、while、return、赋值、表达式等
 * 4. 完整的表达式：算术、关系、逻辑运算符
 * 5. 数组访问和函数调用
 *
 * 关键设计决策：
 * 1. 优先级表：明确定义了运算符的优先级关系
 * 2. 左结合性：通过在递归调用时使用prec+1实现
 * 3. 错误处理：通过异常机制报告语法错误
 * 4. 行号跟踪：在AST节点中保存行号信息用于后续阶段的错误报告
 *
 * 这个实现展示了编译器前端中词法分析和语法分析的完整配合过程，
 * 是理解编译原理的重要实例。
 */
