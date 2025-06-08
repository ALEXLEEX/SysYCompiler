/**
 * 语法分析器头文件
 * 定义了SysY语言的手写递归下降语法分析器类
 */
#ifndef RD_PARSER_HPP
#define RD_PARSER_HPP
#include "../ast/tree.hpp"
#include "lexer.hpp"

/**
 * Parser类：SysY语言的递归下降语法分析器
 * 负责将Token流转换为抽象语法树(AST)
 */
class Parser {
 public:
  /**
   * 构造函数
   * @param lex 词法分析器引用
   */
  explicit Parser(Lexer &lex);

  /**
   * 开始语法分析，解析整个编译单元
   * @return 编译单元的AST节点
   */
  AST::NodePtr parse();

 private:
  Lexer &lex;  // 词法分析器引用
  Token tok;   // 当前Token

  /**
   * 消费一个指定类型的Token
   * @param t 期望的Token类型
   * @return 被消费的Token
   * @throws std::runtime_error 如果当前Token类型不匹配
   */
  Token consume(TokenType t);

  /**
   * 检查当前Token是否为指定类型
   * @param t 要检查的Token类型
   * @return 是否匹配
   */
  bool match(TokenType t);

  /**
   * 前进到下一个Token
   */
  void advance();

  // 各种语法成分的解析函数

  /**
   * 解析编译单元 (CompUnit)
   * CompUnit -> (Decl | FuncDef)*
   */
  AST::CompUnitPtr parseCompUnit();

  /**
   * 解析变量声明 (VarDecl)
   * VarDecl -> BType VarDef {',' VarDef} ';'
   */
  AST::VarDeclPtr parseVarDecl();

  /**
   * 解析变量定义 (VarDef)
   * VarDef -> Ident {'[' ConstExp ']'} ['=' InitVal]
   */
  AST::VarDefPtr parseVarDef();

  /**
   * 解析数组维度
   * @param dims 存储维度大小的向量
   */
  void parseArrayDims(std::vector<int> &dims);

  /**
   * 解析初始值 (InitVal)
   * InitVal -> Exp | '{' [InitVal {',' InitVal}] '}'
   */
  AST::InitValPtr parseInitVal();

  /**
   * 解析函数定义 (FuncDef)
   * FuncDef -> FuncType Ident '(' [FuncFParams] ')' Block
   */
  AST::FuncDefPtr parseFuncDef();

  /**
   * 解析函数形参列表 (FuncFParams)
   * FuncFParams -> FuncFParam {',' FuncFParam}
   */
  std::vector<AST::ParamPtr> parseFuncFParams();

  /**
   * 解析函数形参 (FuncFParam)
   * FuncFParam -> BType Ident ['[' ']' {'[' ConstExp ']'}]
   */
  AST::ParamPtr parseFuncFParam();

  /**
   * 解析语句块 (Block)
   * Block -> '{' {BlockItem} '}'
   */
  AST::BlockPtr parseBlock();

  /**
   * 解析语句块项 (BlockItem)
   * BlockItem -> Decl | Stmt
   */
  AST::NodePtr parseBlockItem();

  /**
   * 解析语句 (Stmt)
   * 包括赋值语句、表达式语句、if语句、while语句、return语句等
   */
  AST::NodePtr parseStmt();

  /**
   * 解析表达式 (Exp)
   * Exp -> AddExp
   */
  AST::NodePtr parseExp();

  /**
   * 解析条件表达式 (Cond)
   * Cond -> LOrExp
   */
  AST::NodePtr parseCond();

  /**
   * 解析二元表达式（使用运算符优先级）
   * @param prec 当前优先级
   * @return 表达式AST节点
   */
  AST::NodePtr parseBinary(int prec);

  /**
   * 解析一元表达式 (UnaryExp)
   * UnaryExp -> PrimaryExp | Ident '(' [FuncRParams] ')' | UnaryOp UnaryExp
   */
  AST::NodePtr parseUnary();

  /**
   * 解析基本表达式 (PrimaryExp)
   * PrimaryExp -> '(' Exp ')' | LVal | Number
   */
  AST::NodePtr parsePrimary();

  /**
   * 获取运算符的优先级
   * @param t Token类型
   * @return 优先级（数字越大优先级越高）
   */
  int precedence(TokenType t);

  /**
   * 将Token类型转换为二元运算符类型
   * @param t Token类型
   * @return 二元运算符类型
   */
  BinaryOp toBinaryOp(TokenType t);
};

#endif

/**
 * 总结：
 * 这个头文件定义了SysY语言的递归下降语法分析器类Parser。
 *
 * 核心功能：
 * 1. 将词法分析器产生的Token流转换为抽象语法树（AST）
 * 2. 实现递归下降分析算法
 * 3. 处理各种语法结构：声明、语句、表达式等
 *
 * 设计特点：
 * 1. 每个语法规则对应一个解析函数
 * 2. 使用运算符优先级分析法处理表达式
 * 3. 支持错误检测和报告
 * 4. 生成符合AST接口的语法树节点
 *
 * 主要解析函数：
 * - parseCompUnit：解析整个编译单元
 * - parseStmt：解析各种语句
 * - parseBinary：处理二元表达式的优先级
 * - parseUnary/parsePrimary：处理一元表达式和基本表达式
 *
 * 辅助函数：
 * - consume：消费指定类型的Token
 * - match：检查Token类型
 * - precedence：获取运算符优先级
 */
