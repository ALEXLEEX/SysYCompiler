#include "lexer.hpp"
#include <cctype>
#include <stdexcept>

Lexer::Lexer(std::istream &in) : input(in), has_peek(false), line(1) {}

char Lexer::get() {
  char c = input.get();
  if (c == '\n') line++;
  return c;
}

char Lexer::peek_char() {
  return input.peek();
}

void Lexer::skip_blank() {
  while (true) {
    char c = peek_char();
    if (c == ' ' || c == '\t' || c == '\r' || c == '\n') {
      get();
      continue;
    }
    if (c == '/') {
      get();
      char n = peek_char();
      if (n == '/') {
        while (peek_char() != '\n' && input.good()) get();
      } else if (n == '*') {
        get();
        while (input.good()) {
          char d = get();
          if (d == '*' && peek_char() == '/') {
            get();
            break;
          }
        }
      } else {
        input.unget();
        return;
      }
      continue;
    }
    break;
  }
}

Token Lexer::next() {
  if (has_peek) {
    has_peek = false;
    return current;
  }
  skip_blank();
  Token tok{};
  tok.lineno = line;
  int c = input.peek();
  if (!input.good()) { tok.type = TokenType::END; return tok; }
  // identifiers or keywords
  if (std::isalpha(c) || c == '_') {
    std::string str;
    while (std::isalnum(peek_char()) || peek_char() == '_') {
      str += get();
    }
    tok.str = str;
    if (str == "int") tok.type = TokenType::INT;
    else if (str == "void") tok.type = TokenType::VOID;
    else if (str == "if") tok.type = TokenType::IF;
    else if (str == "else") tok.type = TokenType::ELSE;
    else if (str == "while") tok.type = TokenType::WHILE;
    else if (str == "return") tok.type = TokenType::RETURN;
    else { tok.type = TokenType::IDENT; }
    current = tok; return tok;
  }
  // numbers
  if (std::isdigit(c)) {
    std::string str; str += get();
    if (str[0] == '0') {
      if (peek_char()=='x' || peek_char()=='X') {
        str += get();
        while (std::isxdigit(peek_char())) str += get();
        tok.value = std::stoi(str, nullptr, 16);
      } else {
        while (peek_char()>='0' && peek_char()<='7') str += get();
        tok.value = std::stoi(str, nullptr, 8);
      }
    } else {
      while (std::isdigit(peek_char())) str += get();
      tok.value = std::stoi(str, nullptr, 10);
    }
    tok.type = TokenType::INTCONST;
    current = tok; return tok;
  }
  // operators and punctuations
  char ch = get();
  switch (ch) {
    case '+': tok.type = TokenType::ADD; break;
    case '-': tok.type = TokenType::SUB; break;
    case '*': tok.type = TokenType::MUL; break;
    case '/': tok.type = TokenType::DIV; break;
    case '%': tok.type = TokenType::MOD; break;
    case ';': tok.type = TokenType::SEMICOLON; break;
    case ',': tok.type = TokenType::COMMA; break;
    case '(': tok.type = TokenType::LPAREN; break;
    case ')': tok.type = TokenType::RPAREN; break;
    case '{': tok.type = TokenType::LBRACE; break;
    case '}': tok.type = TokenType::RBRACE; break;
    case '[': tok.type = TokenType::LBRACK; break;
    case ']': tok.type = TokenType::RBRACK; break;
    case '=':
      if (peek_char() == '=') {
        get();
        tok.type = TokenType::EQ;
      } else {
        tok.type = TokenType::ASSIGN;
      }
      break;
    case '!':
      if (peek_char() == '=') { get(); tok.type = TokenType::NEQ; }
      else { tok.type = TokenType::NOT; }
      break;
    case '<':
      if (peek_char() == '=') {
        get();
        tok.type = TokenType::LE;
      } else {
        tok.type = TokenType::LT;
      }
      break;
    case '>':
      if (peek_char() == '=') {
        get();
        tok.type = TokenType::GE;
      } else {
        tok.type = TokenType::GT;
      }
      break;
    case '&':
      if (peek_char() == '&') {
        get();
        tok.type = TokenType::AND;
        break;
      }
      break;
    case '|':
      if (peek_char() == '|') {
        get();
        tok.type = TokenType::OR;
        break;
      }
      break;
    default:
      throw std::runtime_error(std::string("Unknown char ") + ch);
  }
  current = tok; return tok;
}

Token Lexer::peek() {
  if (!has_peek) {
    current = next();
    has_peek = true;
  }
  return current;
}
