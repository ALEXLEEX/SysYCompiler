/**
 * 词法分析器头文件
 * 定义了SysY语言的手写词法分析器类
 */
#ifndef RD_LEXER_HPP
#define RD_LEXER_HPP
#include <istream>
#include <string>

#include "token.hpp"

/**
 * Lexer类：SysY语言的词法分析器
 * 负责将输入的字符流转换为Token流
 */
class Lexer {
 public:
  /**
   * 构造函数
   * @param in 输入字符流
   */
  explicit Lexer(std::istream &in);

  /**
   * 获取下一个Token
   * @return 下一个Token
   */
  Token next();

  /**
   * 预读下一个Token但不消费它
   * @return 下一个Token
   */
  Token peek();

 private:
  std::istream &input;  // 输入字符流的引用
  Token current;        // 当前Token（用于peek功能）
  bool has_peek;        // 是否已经预读了一个Token
  int line;             // 当前行号

  /**
   * 跳过空白字符和注释
   */
  void skip_blank();

  /**
   * 从输入流中获取一个字符，并更新行号
   * @return 读取的字符
   */
  char get();

  /**
   * 预读下一个字符但不消费它
   * @return 下一个字符
   */
  char peek_char();
};

#endif

/**
 * 总结：
 * 这个头文件定义了SysY语言的词法分析器类Lexer。
 *
 * 主要功能：
 * 1. 从输入字符流中识别Token
 * 2. 支持向前看一个Token的功能（peek）
 * 3. 自动跳过空白字符和注释
 * 4. 维护行号信息用于错误报告
 *
 * 设计特点：
 * - 使用手写的有限状态自动机
 * - 支持单行注释（//）和多行注释
 * - 实现了Token的缓存机制支持peek操作
 * - 错误处理通过异常机制
 */