#ifndef RD_PARSER_HPP
#define RD_PARSER_HPP
#include "lexer.hpp"
#include "../ast/tree.hpp"

class Parser {
 public:
  explicit Parser(Lexer &lex);
  AST::NodePtr parse();

 private:
  Lexer &lex;
  Token tok;

  Token consume(TokenType t);
  bool match(TokenType t);
  void advance();

  AST::CompUnitPtr parseCompUnit();
  AST::VarDeclPtr parseVarDecl();
  AST::VarDefPtr parseVarDef();
  void parseArrayDims(std::vector<int> &dims);
  AST::InitValPtr parseInitVal();
  AST::FuncDefPtr parseFuncDef();
  std::vector<AST::ParamPtr> parseFuncFParams();
  AST::ParamPtr parseFuncFParam();
  AST::BlockPtr parseBlock();
  AST::NodePtr parseBlockItem();
  AST::NodePtr parseStmt();
  AST::NodePtr parseExp();

  AST::NodePtr parseCond();
  AST::NodePtr parseBinary(int prec);
  AST::NodePtr parseUnary();
  AST::NodePtr parsePrimary();

  int precedence(TokenType t);
  BinaryOp toBinaryOp(TokenType t);
};

#endif
