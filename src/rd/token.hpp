#ifndef RD_TOKEN_HPP
#define RD_TOKEN_HPP
#include <string>

enum class TokenType {
  END,
  INT, VOID, IF, ELSE, WHILE, RETURN,
  IDENT, INTCONST,
  ADD, SUB, MUL, DIV, MOD, ASSIGN,
  EQ, NEQ, LT, GT, LE, GE,
  AND, OR, NOT,
  LPAREN, RPAREN,
  LBRACE, RBRACE,
  LBRACK, RBRACK,
  SEMICOLON, COMMA
};

struct Token {
  TokenType type;
  std::string str;
  int value;
  int lineno;
};

#endif
