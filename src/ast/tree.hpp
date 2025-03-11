#ifndef AST_TREE_HPP
#define AST_TREE_HPP

#include <iostream>
#include <memory>
#include <string>
#include <vector>

#include "common.hpp"

extern int yylineno;

namespace AST {

// AST 抽象语法树
// Node 是抽象语法树所有节点的基类
class Node;
using NodePtr = std::shared_ptr<Node>;
class Node {
 public:
  //存储节点所在的(结束)行号
  int lineno;

  virtual std::vector<NodePtr> get_children() { return std::vector<NodePtr>(); }
  void print_tree(std::string prefix = "", std::string info_prefix = "");
  virtual std::string to_string() = 0;

  Node() : lineno(yylineno) {}
  virtual ~Node() = default;
};


class IntConst;
using IntConstPtr = std::shared_ptr<IntConst>;
class IntConst : public Node {
 public:
  int value;
  IntConst(int value) : value(value) {}
  std::string to_string() override {
    return "IntConst <value: " + std::to_string(value) + ">";
  }
};

class LVal;
using LValPtr = std::shared_ptr<LVal>;
class LVal : public Node {
 public:
  std::string ident;
#warning Have not support array yet
  LVal(std::string ident) : ident(ident) {}
  std::string to_string() override { return "LVal <ident: " + ident + ">"; }
};

class UnaryExp;
using UnaryExpPtr = std::shared_ptr<UnaryExp>;
class UnaryExp : public Node {
 public:
  BinaryOp op;  // 注意：这里用 BinaryOp 来表示一元运算可能会有歧义
  NodePtr exp;
  UnaryExp(BinaryOp op, NodePtr exp) : op(op), exp(exp) {}
  std::string to_string() override {
    return "UnaryExp <op: " + std::string(op_to_string(op)) + ">";
  }
  std::vector<NodePtr> get_children() override { return {exp}; }
};

class BinaryExp;
using BinaryExpPtr = std::shared_ptr<BinaryExp>;
class BinaryExp : public Node {
 public:
  BinaryOp op;
  NodePtr left, right;

  BinaryExp(BinaryOp op, NodePtr left, NodePtr right)
      : op(op), left(left), right(right) {}
  std::string to_string() override {
    return "BinaryExp <op: " + std::string(op_to_string(op)) + ">";
  }
  std::vector<NodePtr> get_children() override { return {left, right}; }
};

class FuncCall;
using FuncCallPtr = std::shared_ptr<FuncCall>;
class FuncCall : public Node {
 public:
  std::string name;
  std::vector<NodePtr> args;
  FuncCall(const char *name) : name(name) {}
  FuncCall(NodePtr exp) { add_arg(exp); }
  void add_arg(NodePtr exp) { args.push_back(exp); }
  std::string to_string() override { return "FuncCall <name: " + name + ">"; }
  std::vector<NodePtr> get_children() override { return args; }
};

class Block;
using BlockPtr = std::shared_ptr<Block>;
class Block : public Node {
 public:
  std::vector<NodePtr> stmts;
  Block() {}
  Block(NodePtr stmt) { add_stmt(stmt); }
  void add_stmt(NodePtr stmt) { stmts.push_back(stmt); }
  std::string to_string() override { return "Block"; }
  std::vector<NodePtr> get_children() override { return stmts; }
};

class AssignStmt;
using AssignStmtPtr = std::shared_ptr<AssignStmt>;
class AssignStmt : public Node {
 public:
  LValPtr lval;
  NodePtr exp;
  AssignStmt(LValPtr lval, NodePtr exp) : lval(lval), exp(exp) {}
  std::string to_string() override { return "AssignStmt"; }
  std::vector<NodePtr> get_children() override { return {lval, exp}; }
};

class ReturnStmt;
using ReturnStmtPtr = std::shared_ptr<ReturnStmt>;
class ReturnStmt : public Node {
 public:
  NodePtr exp;
  ReturnStmt() : exp(nullptr) {}
  ReturnStmt(NodePtr exp) : exp(exp) {}
  std::string to_string() override { return "ReturnStmt"; }
  std::vector<NodePtr> get_children() override {
    return exp ? std::vector<NodePtr>{exp} : std::vector<NodePtr>();
  }
};

class VarDef;
using VarDefPtr = std::shared_ptr<VarDef>;
class VarDef : public Node {
 public:
  std::string ident;
  VarDef(const char *ident) : ident(ident) {}
  // modify
  int initVal;
  VarDef(const char *ident, int initVal) : ident(ident), initVal(initVal) {}
  std::string to_string() override { return "VarDef <ident: " + ident + ">"; }
};

class VarDecl;
using VarDeclPtr = std::shared_ptr<VarDecl>;
class VarDecl : public Node {
 public:
  BasicType btype;
  std::vector<VarDefPtr> defs;
  VarDecl(VarDefPtr def) : btype(BasicType::Unknown) { add_def(def); }
  void add_def(VarDefPtr def) { defs.push_back(def); }
  std::string to_string() override {
    return "VarDecl <btype: " + std::string(type_to_string(btype)) + ">";
  }
  std::vector<NodePtr> get_children() override {
    // 将 VarDefPtr 转为 NodePtr 以便打印
    return std::vector<NodePtr>(defs.begin(), defs.end());
  }
};

class FuncDef;
using FuncDefPtr = std::shared_ptr<FuncDef>;
class FuncDef : public Node {
 public:
  BasicType return_btype;
  std::string name;
#warning Have not support params yet
  BlockPtr block;
  FuncDef(BasicType return_btype, const char *name, BlockPtr block)
      : return_btype(return_btype), name(name), block(block) {}
  std::string to_string() override {
    return "FuncDef <return_btype: " +
           std::string(type_to_string(return_btype)) + ", name: " + name + ">";
  }
  std::vector<NodePtr> get_children() override { return {block}; }
};

class CompUnit;
using CompUnitPtr = std::shared_ptr<CompUnit>;
class CompUnit : public Node {
 public:
  std::vector<NodePtr> units;  // FuncDef or VarDecl
  CompUnit(NodePtr unit) { add_unit(unit); }
  void add_unit(NodePtr unit) { units.push_back(unit); }
  std::string to_string() override { return "CompUnit"; }
  std::vector<NodePtr> get_children() override { return units; }
};

//---------------------------------
// 下面是新增的节点
//---------------------------------

// 1. 表达式语句节点（单纯的一条“Exp;”语句）
// 有时也称为 "ExpStmt"
class ExpStmt;
using ExpStmtPtr = std::shared_ptr<ExpStmt>;
class ExpStmt : public Node {
 public:
  NodePtr exp;  // 这里可以是任意表达式
  ExpStmt() : exp(nullptr) {}
  ExpStmt(NodePtr exp) : exp(exp) {}
  std::string to_string() override { return "ExpStmt"; }
  std::vector<NodePtr> get_children() override { 
    if (exp)
    {
      return {exp};
    }
    
    return std::vector<NodePtr>(); 
  }
};

// 2. 函数形式参数节点（Param），可以用于表示形参信息
class Param;
using ParamPtr = std::shared_ptr<Param>;
class Param : public Node {
 public:
  BasicType btype;
  std::string ident;
  // 这里暂不实现数组形参的维数，视需求可自行扩展
  Param(BasicType btype, const char *ident) : btype(btype), ident(ident) {}
  std::string to_string() override {
    return "Param <btype: " + std::string(type_to_string(btype)) +
           ", ident: " + ident + ">";
  }
};

// 3. IfStmt 节点，用于表示 if / if-else
class IfStmt;
using IfStmtPtr = std::shared_ptr<IfStmt>;
class IfStmt : public Node {
 public:
  NodePtr condition;     // 条件表达式
  NodePtr then_stmt;     // if 分支
  NodePtr else_stmt;     // else 分支（可选）
  IfStmt(NodePtr cond, NodePtr then_stmt, NodePtr else_stmt = nullptr)
      : condition(cond), then_stmt(then_stmt), else_stmt(else_stmt) {}
  std::string to_string() override { return "IfStmt"; }
  std::vector<NodePtr> get_children() override {
    if (else_stmt) {
      return {condition, then_stmt, else_stmt};
    } else {
      return {condition, then_stmt};
    }
  }
};

// 4. WhileStmt 节点，用于表示 while
class WhileStmt;
using WhileStmtPtr = std::shared_ptr<WhileStmt>;
class WhileStmt : public Node {
 public:
  NodePtr condition;  // 循环条件
  NodePtr body;       // 循环体
  WhileStmt(NodePtr cond, NodePtr body) : condition(cond), body(body) {}
  std::string to_string() override { return "WhileStmt"; }
  std::vector<NodePtr> get_children() override {
    return {condition, body};
  }
};

}  // namespace AST

#endif  // AST_TREE_HPP