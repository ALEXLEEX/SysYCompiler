/**
 * Token定义头文件
 * 定义了SysY语言词法分析器使用的Token类型和Token结构体
 */
#ifndef RD_TOKEN_HPP
#define RD_TOKEN_HPP
#include <string>

/**
 * TokenType枚举：定义了SysY语言中所有可能的Token类型
 */
enum class TokenType {
  END,  // 文件结束标记

  // 关键字 (Keywords)
  INT,     // int关键字
  VOID,    // void关键字
  IF,      // if关键字
  ELSE,    // else关键字
  WHILE,   // while关键字
  RETURN,  // return关键字

  // 标识符和常量 (Identifiers and Constants)
  IDENT,     // 标识符（变量名、函数名等）
  INTCONST,  // 整数常量

  // 算术运算符 (Arithmetic Operators)
  ADD,     // + 加法
  SUB,     // - 减法
  MUL,     // * 乘法
  DIV,     // / 除法
  MOD,     // % 取模
  ASSIGN,  // = 赋值

  // 关系运算符 (Relational Operators)
  EQ,   // == 等于
  NEQ,  // != 不等于
  LT,   // < 小于
  GT,   // > 大于
  LE,   // <= 小于等于
  GE,   // >= 大于等于

  // 逻辑运算符 (Logical Operators)
  AND,  // && 逻辑与
  OR,   // || 逻辑或
  NOT,  // ! 逻辑非

  // 分隔符 (Delimiters)
  LPAREN,     // ( 左圆括号
  RPAREN,     // ) 右圆括号
  LBRACE,     // { 左大括号
  RBRACE,     // } 右大括号
  LBRACK,     // [ 左方括号
  RBRACK,     // ] 右方括号
  SEMICOLON,  // ; 分号
  COMMA       // , 逗号
};

/**
 * Token结构体：表示一个词法单元
 */
struct Token {
  TokenType type;   // Token的类型
  std::string str;  // Token的字符串值（用于标识符）
  int value;        // Token的数值（用于整数常量）
  int lineno;       // Token所在的行号（用于错误报告）
};

#endif

/**
 * 总结：
 * 这个文件定义了SysY语言词法分析中使用的所有Token类型和Token结构体。
 *
 * TokenType枚举包含了：
 * - 关键字：int, void, if, else, while, return
 * - 标识符和常量：IDENT（变量名、函数名），INTCONST（整数常量）
 * - 运算符：算术运算符（+, -, *, /, %），关系运算符（==, !=, <, >, <=, >=），
 *          逻辑运算符（&&, ||, !），赋值运算符（=）
 * - 分隔符：各种括号和标点符号
 *
 * Token结构体存储了：
 * - type：Token的类型
 * - str：Token的字符串值（用于标识符）
 * - value：Token的数值（用于整数常量）
 * - lineno：Token所在的行号（用于错误报告和调试）
 */
