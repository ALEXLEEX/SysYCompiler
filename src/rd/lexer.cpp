/**
 * SysY语言词法分析器实现
 * 负责将输入的字符流转换为Token流，识别关键字、标识符、常量、运算符等
 */
#include "lexer.hpp"

#include <cctype>
#include <stdexcept>

/**
 * 构造函数：初始化词法分析器
 * @param in 输入字符流
 */
Lexer::Lexer(std::istream &in) : input(in), has_peek(false), line(1) {}

/**
 * 从输入流获取一个字符，并自动处理换行符以更新行号
 * @return 读取的字符
 */
char Lexer::get() {
  char c = input.get();
  if (c == '\n') {
    line++;  // 遇到换行符时行号递增
  }
  return c;
}

/**
 * 预读下一个字符但不从输入流中移除它
 * @return 下一个字符
 */
char Lexer::peek_char() { return input.peek(); }

/**
 * 跳过空白字符（空格、制表符、换行符等）和注释
 * 支持两种注释格式：
 * 1. 单行注释：// ... \n
 * 2. 多行注释
*/
void Lexer::skip_blank() {
  while (true) {
    char c = peek_char();

    // 跳过空白字符
    if (c == ' ' || c == '\t' || c == '\r' || c == '\n') {
      get();
      continue;
    }

    // 处理注释
    if (c == '/') {
      get();  // 消费 '/'
      char n = peek_char();

      if (n == '/') {  // 单行注释 //
        while (peek_char() != '\n' && input.good()) {
          get();
        }
      } else if (n == '*') {  // 多行注释 /* */
        get();                // 消费 '*'
        while (input.good()) {
          char d = get();
          if (d == '*' && peek_char() == '/') {
            get();  // 消费结束的 '/'
            break;
          }
        }
      } else {
        // 如果不是注释，将 '/' 放回输入流
        input.unget();
        return;
      }
      continue;
    }
    break;  // 既不是空白字符也不是注释，退出循环
  }
}

/**
 * 获取下一个Token的主要函数
 * 处理Token的识别逻辑，包括关键字、标识符、数字常量、运算符等
 * @return 识别出的Token
 */
Token Lexer::next() {
  // 如果之前调用了peek()，直接返回已经预读的Token
  if (has_peek) {
    has_peek = false;
    return current;
  }

  skip_blank();       // 跳过空白字符和注释
  Token tok{};        // 创建新的Token
  tok.lineno = line;  // 设置Token的行号

  int c = input.peek();  // 预读下一个字符
  if (!input.good()) {
    tok.type = TokenType::END;
    return tok;
  }

  // 识别标识符或关键字
  // 标识符以字母或下划线开头，后跟字母、数字或下划线
  if (std::isalpha(c) || c == '_') {
    std::string str;
    while (std::isalnum(peek_char()) || peek_char() == '_') {
      str += get();
    }
    tok.str = str;

    // 检查是否为关键字
    if (str == "int") {
      tok.type = TokenType::INT;
    } else if (str == "void") {
      tok.type = TokenType::VOID;
    } else if (str == "if") {
      tok.type = TokenType::IF;
    } else if (str == "else") {
      tok.type = TokenType::ELSE;
    } else if (str == "while") {
      tok.type = TokenType::WHILE;
    } else if (str == "return") {
      tok.type = TokenType::RETURN;
    } else {
      tok.type = TokenType::IDENT;  // 不是关键字，则为标识符
    }
    current = tok;
    return tok;
  }
  // 识别数字常量
  // 支持十进制、八进制（0开头）和十六进制（0x开头）
  if (std::isdigit(c)) {
    std::string str;
    str += get();  // 获取第一个数字字符

    if (str[0] == '0') {  // 以0开头，可能是八进制或十六进制
      if (peek_char() == 'x' || peek_char() == 'X') {  // 十六进制 0x...
        str += get();                                  // 消费 'x' 或 'X'
        while (std::isxdigit(peek_char())) {
          str += get();  // 读取十六进制数字
        }
        tok.value = std::stoi(str, nullptr, 16);  // 按十六进制解析
      } else {                                    // 八进制 0...
        while (peek_char() >= '0' && peek_char() <= '7') {
          str += get();  // 读取八进制数字
        }
        tok.value = std::stoi(str, nullptr, 8);  // 按八进制解析
      }
    } else {  // 十进制数字
      while (std::isdigit(peek_char())) {
        str += get();  // 读取十进制数字
      }
      tok.value = std::stoi(str, nullptr, 10);  // 按十进制解析
    }
    tok.type = TokenType::INTCONST;
    current = tok;
    return tok;
  }
  // 识别运算符和标点符号
  char ch = get();  // 获取当前字符
  switch (ch) {
    // 算术运算符
    case '+':
      tok.type = TokenType::ADD;
      break;
    case '-':
      tok.type = TokenType::SUB;
      break;
    case '*':
      tok.type = TokenType::MUL;
      break;
    case '/':
      tok.type = TokenType::DIV;
      break;
    case '%':
      tok.type = TokenType::MOD;
      break;

    // 标点符号
    case ';':
      tok.type = TokenType::SEMICOLON;
      break;
    case ',':
      tok.type = TokenType::COMMA;
      break;
    case '(':
      tok.type = TokenType::LPAREN;
      break;
    case ')':
      tok.type = TokenType::RPAREN;
      break;
    case '{':
      tok.type = TokenType::LBRACE;
      break;
    case '}':
      tok.type = TokenType::RBRACE;
      break;
    case '[':
      tok.type = TokenType::LBRACK;
      break;
    case ']':
      tok.type = TokenType::RBRACK;
      break;

    // 赋值和相等运算符
    case '=':
      if (peek_char() == '=') {  // == 相等比较
        get();
        tok.type = TokenType::EQ;
      } else {  // = 赋值
        tok.type = TokenType::ASSIGN;
      }
      break;

    // 逻辑非和不等运算符
    case '!':
      if (peek_char() == '=') {  // != 不等比较
        get();
        tok.type = TokenType::NEQ;
      } else {  // ! 逻辑非
        tok.type = TokenType::NOT;
      }
      break;

    // 小于和小于等于运算符
    case '<':
      if (peek_char() == '=') {  // <= 小于等于
        get();
        tok.type = TokenType::LE;
      } else {  // < 小于
        tok.type = TokenType::LT;
      }
      break;

    // 大于和大于等于运算符
    case '>':
      if (peek_char() == '=') {  // >= 大于等于
        get();
        tok.type = TokenType::GE;
      } else {  // > 大于
        tok.type = TokenType::GT;
      }
      break;

    // 逻辑与运算符
    case '&':
      if (peek_char() == '&') {  // && 逻辑与
        get();
        tok.type = TokenType::AND;
        break;
      }
      break;

    // 逻辑或运算符
    case '|':
      if (peek_char() == '|') {  // || 逻辑或
        get();
        tok.type = TokenType::OR;
        break;
      }
      break;

    default:
      // 遇到未知字符时抛出异常
      throw std::runtime_error(std::string("Unknown char ") + ch);
  }
  current = tok;
  return tok;
}

/**
 * 预读下一个Token但不消费它
 * 实现向前看一个Token的功能，用于语法分析时的决策
 * @return 下一个Token
 */
Token Lexer::peek() {
  if (!has_peek) {
    current = next();  // 如果还没有预读，则获取下一个Token
    has_peek = true;   // 标记已经预读
  }
  return current;
}

/**
 * 总结：
 * 这个文件实现了SysY语言的手写词法分析器。
 *
 * 主要特性：
 * 1. 完整的Token识别：关键字、标识符、整数常量、运算符、分隔符
 * 2. 多种数制支持：十进制、八进制（0开头）、十六进制（0x开头）
 * 3. 注释处理：单行注释（//）和多行注释
 * 4. 向前看功能：支持peek操作，便于语法分析器进行决策
 * 5. 行号跟踪：自动维护当前行号，用于错误报告
 * 
 * 实现方法：
 * - 使用有限状态自动机的思想
 * - 逐字符读取和分析
 * - 通过peek_char()实现向前看字符
 * - 通过has_peek标志实现Token级别的向前看
 * 
 * 错误处理：
 * - 遇到未知字符时抛出运行时异常
 * - 包含详细的错误信息
 */
