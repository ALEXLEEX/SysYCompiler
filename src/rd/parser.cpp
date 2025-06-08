#include "parser.hpp"
#include <stdexcept>
using namespace AST;

Parser::Parser(Lexer &l) : lex(l) { advance(); }

void Parser::advance() { tok = lex.next(); }

bool Parser::match(TokenType t) { return tok.type == t; }

Token Parser::consume(TokenType t) {
  if (tok.type != t) throw std::runtime_error("Unexpected token");
  Token cur = tok; advance(); return cur;
}

NodePtr Parser::parse() { return parseCompUnit(); }

CompUnitPtr Parser::parseCompUnit() {
  auto cu = std::make_shared<CompUnit>(nullptr);
  cu->units.clear();
  while (!match(TokenType::END)) {
    if (match(TokenType::INT) || match(TokenType::VOID)) {
      TokenType t = tok.type; advance();
      Token idTok = consume(TokenType::IDENT);
      if (match(TokenType::LPAREN)) {
        // function def
        advance();
        std::vector<ParamPtr> params;
        if (!match(TokenType::RPAREN)) {
          params = parseFuncFParams();
        }
        consume(TokenType::RPAREN);
        auto block = parseBlock();
        BasicType ret = (t == TokenType::INT) ? BasicType::Int : BasicType::Void;
        auto f = std::make_shared<FuncDef>(ret, idTok.str.c_str(), block, params);
        cu->add_unit(f);
      } else {
        // var decl start with parsed type and first ident already read
        std::vector<VarDefPtr> defs;
        // treat first ident as part of VarDef; but parseVarDecl expects type token not consumed
        // We'll reconstruct var decl manually
        // parse var def rest
        std::vector<int> dims; // dims after ident
        while (match(TokenType::LBRACK)) {
          advance();
          auto eTok = consume(TokenType::INTCONST);
          dims.push_back(eTok.value);
          consume(TokenType::RBRACK);
        }
        InitValPtr init = nullptr;
        if (match(TokenType::ASSIGN)) {
          advance();
          init = parseInitVal();
        }
        auto var = std::make_shared<VarDef>(idTok.str.c_str());
        for (int d : dims) var->add_dim(d);
        if (init) var->add_initVal(init);
        defs.push_back(var);
        while (match(TokenType::COMMA)) {
          advance();
          defs.push_back(parseVarDef());
        }
        consume(TokenType::SEMICOLON);
        auto vd = std::make_shared<VarDecl>(defs.front());
        for (size_t i=1;i<defs.size();++i) vd->add_def(defs[i]);
        vd->btype = BasicType::Int; // only int
        cu->add_unit(vd);
      }
    } else {
      throw std::runtime_error("Unexpected token in compilation unit");
    }
  }
  return cu;
}

VarDefPtr Parser::parseVarDef() {
  Token idTok = consume(TokenType::IDENT);
  std::vector<int> dims;
  while (match(TokenType::LBRACK)) {
    advance();
    Token val = consume(TokenType::INTCONST);
    consume(TokenType::RBRACK);
    dims.push_back(val.value);
  }
  InitValPtr init = nullptr;
  if (match(TokenType::ASSIGN)) { advance(); init = parseInitVal(); }
  auto var = std::make_shared<VarDef>(idTok.str.c_str());
  for (int d : dims) var->add_dim(d);
  if (init) var->add_initVal(init);
  return var;
}

void Parser::parseArrayDims(std::vector<int> &dims) {
  while (match(TokenType::LBRACK)) {
    advance();
    Token val = consume(TokenType::INTCONST);
    consume(TokenType::RBRACK);
    dims.push_back(val.value);
  }
}

InitValPtr Parser::parseInitVal() {
  if (match(TokenType::LBRACE)) {
    advance();
    if (match(TokenType::RBRACE)) { advance(); return std::make_shared<InitVal>(); }
      auto first = parseInitVal();
      std::vector<InitValPtr> vals;
      vals.push_back(first);
      while (match(TokenType::COMMA)) {
        advance();
        vals.push_back(parseInitVal());
      }
      consume(TokenType::RBRACE);
      auto list = std::make_shared<InitVal>(vals.front());
      for (size_t i = 1; i < vals.size(); ++i) {
        list->add_sub(vals[i]);
      }
      return list;
  } else {
    auto exp = parseExp();
    return std::make_shared<InitVal>(exp);
  }
}

FuncDefPtr Parser::parseFuncDef() {
  // not used in new parseCompUnit
  return nullptr;
}

std::vector<ParamPtr> Parser::parseFuncFParams() {
  std::vector<ParamPtr> params;
  params.push_back(parseFuncFParam());
  while (match(TokenType::COMMA)) { advance(); params.push_back(parseFuncFParam()); }
  return params;
}

ParamPtr Parser::parseFuncFParam() {
  consume(TokenType::INT);
  Token idTok = consume(TokenType::IDENT);
  std::vector<int> dims;
  if (match(TokenType::LBRACK)) {
    advance();
    consume(TokenType::RBRACK);
    dims.push_back(-1);
    parseArrayDims(dims);
  }
  auto p = std::make_shared<Param>(idTok.str.c_str());
  for (size_t i=0;i<dims.size();++i) {
    if (i==0 && dims[i]==-1) p->add_dim(-1); else p->add_dim(dims[i]);
  }
  return p;
}

BlockPtr Parser::parseBlock() {
  consume(TokenType::LBRACE);
  std::vector<NodePtr> stmts;
  while (!match(TokenType::RBRACE)) {
    stmts.push_back(parseBlockItem());
  }
  consume(TokenType::RBRACE);
  auto blk = std::make_shared<Block>();
  for (auto &s: stmts) blk->add_stmt(s);
  return blk;
}

NodePtr Parser::parseBlockItem() {
  if (match(TokenType::INT)) {
    advance();
    auto first = parseVarDef();
    std::vector<VarDefPtr> defs; defs.push_back(first);
    while (match(TokenType::COMMA)) { advance(); defs.push_back(parseVarDef()); }
    consume(TokenType::SEMICOLON);
    auto vd = std::make_shared<VarDecl>(defs.front());
    for (size_t i=1;i<defs.size();++i) vd->add_def(defs[i]);
    vd->btype = BasicType::Int;
    return vd;
  } else {
    return parseStmt();
  }
}

NodePtr Parser::parseStmt() {
  if (match(TokenType::LBRACE)) return parseBlock();
  if (match(TokenType::IF)) {
    int line = tok.lineno;
    advance(); consume(TokenType::LPAREN); auto cond = parseCond(); consume(TokenType::RPAREN);
    auto then_s = parseStmt();
    if (match(TokenType::ELSE)) { advance(); auto else_s = parseStmt(); auto node = std::make_shared<IfStmt>(cond, then_s, else_s); node->lineno = line; return node; }
    else { auto node = std::make_shared<IfStmt>(cond, then_s); node->lineno = line; return node; }
  }
  if (match(TokenType::WHILE)) {
    int line = tok.lineno;
    advance(); consume(TokenType::LPAREN); auto cond = parseCond(); consume(TokenType::RPAREN);
    auto body = parseStmt();
    auto node = std::make_shared<WhileStmt>(cond, body);
    node->lineno = line;
    return node;
  }
  if (match(TokenType::RETURN)) {
    int line = tok.lineno; advance();
    if (match(TokenType::SEMICOLON)) { advance(); auto node = std::make_shared<ReturnStmt>(); node->lineno = line; return node; }
    auto e = parseExp(); consume(TokenType::SEMICOLON); auto node = std::make_shared<ReturnStmt>(e); node->lineno = line; return node;
  }
  if (match(TokenType::SEMICOLON)) {
    int line = tok.lineno;
    advance();
    auto node = std::make_shared<ExpStmt>();
    node->lineno = line;
    return node;
  }
  NodePtr exp = parseExp();
  if (match(TokenType::ASSIGN)) {
    auto lval = std::dynamic_pointer_cast<LVal>(exp);
    if (!lval) throw std::runtime_error("assign lhs not lval");
    advance();
    int line = tok.lineno;
    auto e = parseExp();
    consume(TokenType::SEMICOLON);
    auto node = std::make_shared<AssignStmt>(lval, e);
    node->lineno = line;
    return node;
  } else {
    consume(TokenType::SEMICOLON);
    return exp;
  }
}

NodePtr Parser::parseExp() {
  return parseBinary(1); // start from lowest precedence
}

NodePtr Parser::parseCond() { return parseBinary(1); }

int Parser::precedence(TokenType t) {
  switch (t) {
    case TokenType::OR: return 1;
    case TokenType::AND: return 2;
    case TokenType::EQ: case TokenType::NEQ: return 3;
    case TokenType::LT: case TokenType::GT: case TokenType::LE: case TokenType::GE: return 4;
    case TokenType::ADD: case TokenType::SUB: return 5;
    case TokenType::MUL: case TokenType::DIV: case TokenType::MOD: return 6;
    default: return 0;
  }
}

BinaryOp Parser::toBinaryOp(TokenType t) {
  switch (t) {
    case TokenType::OR: return BinaryOp::LOr;
    case TokenType::AND: return BinaryOp::LAnd;
    case TokenType::EQ: return BinaryOp::EQ;
    case TokenType::NEQ: return BinaryOp::NEQ;
    case TokenType::LT: return BinaryOp::LT;
    case TokenType::GT: return BinaryOp::GT;
    case TokenType::LE: return BinaryOp::LE;
    case TokenType::GE: return BinaryOp::GE;
    case TokenType::ADD: return BinaryOp::Add;
    case TokenType::SUB: return BinaryOp::Sub;
    case TokenType::MUL: return BinaryOp::Mul;
    case TokenType::DIV: return BinaryOp::Div;
    case TokenType::MOD: return BinaryOp::Mod;
    case TokenType::NOT: return BinaryOp::Not;
    default: return BinaryOp::Add;
  }
}

NodePtr Parser::parseBinary(int prec) {
  NodePtr left = parseUnary();
  while (true) {
    int p = precedence(tok.type);
    if (p < prec || p == 0) break;
    TokenType op = tok.type; int line = tok.lineno; advance();
    NodePtr right = parseBinary(p + 1);
    auto node = std::make_shared<BinaryExp>(toBinaryOp(op), left, right);
    node->lineno = line;
    left = node;
  }
  return left;
}

NodePtr Parser::parseUnary() {
  if (tok.type == TokenType::ADD || tok.type == TokenType::SUB || tok.type == TokenType::NOT) {
    TokenType op = tok.type; int line = tok.lineno; advance();
    auto expr = parseUnary();
    auto node = std::make_shared<UnaryExp>(toBinaryOp(op), expr);
    node->lineno = line;
    return node;
  }
  return parsePrimary();
}

NodePtr Parser::parsePrimary() {
  if (match(TokenType::LPAREN)) {
    int line = tok.lineno;
    advance();
    auto e = parseExp();
    consume(TokenType::RPAREN);
    yylineno = line;
    auto node = std::make_shared<PrimaryExp>(e);
    node->lineno = line;
    return node;
  }
  if (match(TokenType::INTCONST)) {
    int line = tok.lineno;
    int v = tok.value;
    advance();
    yylineno = line;
    auto node = std::make_shared<IntConst>(v);
    node->lineno = line;
    return node;
  }
  if (match(TokenType::IDENT)) {
    std::string name = tok.str;
    int line = tok.lineno;
    advance();
    if (match(TokenType::LPAREN)) {
      advance();
      auto call = std::make_shared<FuncCall>(name.c_str());
      if (!match(TokenType::RPAREN)) {
        call->add_arg(parseExp());
        while (match(TokenType::COMMA)) { advance(); call->add_arg(parseExp()); }
      }
      consume(TokenType::RPAREN);
      call->lineno = line;
      return call;
    } else {
      auto lval = std::make_shared<LVal>(name);
      while (match(TokenType::LBRACK)) {
        advance();
        auto idx = parseExp();
        consume(TokenType::RBRACK);
        lval->add_dim(idx);
      }
      lval->lineno = line;
      return lval;
    }
  }
  throw std::runtime_error("Unexpected token in primary");
}
