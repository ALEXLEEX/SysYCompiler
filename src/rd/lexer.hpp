#ifndef RD_LEXER_HPP
#define RD_LEXER_HPP
#include <string>
#include <istream>
#include "token.hpp"

class Lexer {
 public:
  explicit Lexer(std::istream &in);
  Token next();
  Token peek();

 private:
  std::istream &input;
  Token current;
  bool has_peek;
  int line;

  void skip_blank();
  char get();
  char peek_char();
};

#endif
