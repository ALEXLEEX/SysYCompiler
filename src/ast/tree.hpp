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
  // 增加一个函数 用于获取他的值 为了数组维度服务
  int get_value() { return value; }
  std::string to_string() override {
    return "IntConst <value: " + std::to_string(value) + ">";
  }
};

class LVal;
using LValPtr = std::shared_ptr<LVal>;
class LVal : public Node {
 public:
  std::string ident;
// #warning Have not support array yet
  // 这里存每个下标的表达式 如不是数组 则此项为空
  std::vector<NodePtr> dims;
  LVal(std::string ident) : ident(ident) {}
  // 如果是数组的话 parser.y里面使用下面的构造函数和添加维度函数
  LVal(std::string ident, NodePtr dim) : ident(ident) { dims.push_back(dim); }
  void add_dim(NodePtr dim) { dims.push_back(dim); }
  std::string to_string() override { 
    // 可选地在打印中提示下标维度数
    // 比如 "LVal <ident: arr> (dims=2)"
    return "LVal <ident: " + ident + (dims.empty() 
      ? "" 
      : (", dims=" + std::to_string(dims.size()))) + ">";
  }
  std::vector<NodePtr> get_children() override { 
    // 返回所有下标表达式 这样应该是打印出每个dim的表达式
    return dims;
  }
};

class UnaryExp;
using UnaryExpPtr = std::shared_ptr<UnaryExp>;
class UnaryExp : public Node {
 public:
  BinaryOp op;  // 注意 这里用 BinaryOp 来表示一元运算可能会有歧义
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

class InitVal;
using InitValPtr = std::shared_ptr<InitVal>;
class InitVal : public Node {
 public:
  // 我们用一个 enum 来表示本节点哪种模式
  enum class Kind {
    EXP,     // initVal 是一个表达式
    BRACE    // initVal 是 { initVal, initVal, ... } 复合
  } kind;

  // 如果 kind==EXP, 我们存放 expr
  NodePtr expr; // 这里可以是任意表达式节点
  
  // 如果 kind==BRACE, 我们存放多个子InitVal
  std::vector<InitValPtr> subInitVals;

  // 构造1 表达式
  InitVal(NodePtr e) : kind(Kind::EXP), expr(e) {}

  // 构造2 花括号复合
  InitVal(const std::vector<InitValPtr> &subs)
    : kind(Kind::BRACE), expr(nullptr), subInitVals(subs) {}

  // 构造3 花括号内为空
  InitVal() : kind(Kind::BRACE), expr(nullptr) {}

  // 如果在解析时，需要反复添加 subInitVal, 就加一个 add_sub(InitValPtr)
  void add_sub(const InitValPtr &val) {
    subInitVals.push_back(val);
  }

  std::string to_string() override {
    if (kind == Kind::EXP) {
      return "InitVal(Expr)";
    } else {
      // BRACE
      return "InitVal(Brace) size=" + std::to_string(subInitVals.size());
    }
  }

  std::vector<NodePtr> get_children() override {
    // 如果是表达式，就返回它
    // 如果是复合，返回每个 subInitVal
    if (kind == Kind::EXP) {
      if (expr) return {expr};
      else return {};
    } else {
      // BRACE
      // subInitVals 是 InitValPtr, 但 they are NodePtr
      // we can cast them or store them as NodePtr
      std::vector<NodePtr> children;
      for (auto &val : subInitVals) {
        children.push_back(val);
      }
      return children;
    }
  }
};

class VarDef;
using VarDefPtr = std::shared_ptr<VarDef>;
class VarDef : public Node {
 public:
  std::string ident;
  // 支持数组和符合初值
  std::vector<int> dims;
  // 新增初值节点 如果没有 = 则为空
  InitValPtr initVal;

  // 构造函数1 仅有标识符(标量 无数组 无初值)
  VarDef(const char *ident) : ident(ident), initVal(nullptr) {}

  // 构造函数2 传入初始化器
  VarDef(const char *ident, InitValPtr initVal) : ident(ident), initVal(initVal) {}

  // 构造函数3 传入整维度信息
  VarDef(const char *ident, int dim) : ident(ident), initVal(nullptr) {
    dims.push_back(dim);
  }
  VarDef(int dim) : ident("temp"), initVal(nullptr) {
    dims.push_back(dim);
  }
  // 修改ident
  void set_ident(const char *ident) { this->ident = ident; }
  // 获取dims
  std::vector<int> get_dims() { return dims; }

  // 构造函数4 传入整维度信息 初始化器
  VarDef(const char *ident, int dim, InitValPtr initVal) : ident(ident), initVal(initVal) {
    dims.push_back(dim);
  }
  void add_dim(int dim) { dims.insert(dims.begin(), dim); }
  void add_initVal(InitValPtr initVal) { this->initVal = initVal; }
  std::string to_string() override {
    // 在打印里可以显示 ident dims大小 以及是否有初值
    std::string info = "VarDef <ident: " + ident + ">";
    if (!dims.empty()) {
      info += " [dims=";
      for (size_t i = 0; i < dims.size(); i++) {
        if (i > 0) info += ",";
        info += std::to_string(dims[i]);
      }
      info += "]";
    }
    if (initVal) {
      info += " with InitVal";
    }
    return info;
  }
  std::vector<NodePtr> get_children() override {
    // 若想在打印或遍历AST时看 initVal 结构，把它返回
    // dims 只是存 int 不是 NodePtr
    if (initVal) {
      return {initVal};
    }
    return {};
  }
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

// 实现函数形参 Param 表示形参信息
class Param;
using ParamPtr = std::shared_ptr<Param>;
class Param : public Node {
 public:
  std::string ident;
  // 扩展支持数组维度
  std::vector<int> dims;
  Param(const char *ident) : ident(ident) {}
  Param(const char *ident, int dim) : ident(ident) {
    // 第一维度肯定是 [ ] 空 所以我们直接 push 一个-1
    dims.push_back(dim);
  }
  // 这里我们直接不创建 IntConst 节点了 直接传入 INTCONST 的属性值 int_val
  // 太招笑了家人们 根本不是IntConst 而是ArrayDims
  void add_dim(int dim) { dims.push_back(dim); }
  // 添加维度数组
  void add_dims(const std::vector<int> &dims) {
    this->dims.insert(this->dims.end(), dims.begin(), dims.end());
  }
  std::string to_string() override {
    // 可以在输出中显示每个维度
    // 例如: Param <ident: arr, dims=[null,10,20]>
    std::string info = "Param <ident: ";
    info += ident;

    if (!dims.empty()) {
      info += ", dims=[";
      for (size_t i = 0; i < dims.size(); i++) {
        if (i > 0) info += ",";
        // 用-1表示空尺寸
        if (dims[i] == -1) info += "null";
        else info += std::to_string(dims[i]);
      }
      info += "]";
    }
    info += ">";
    return info;
  }
};

class FuncDef;
using FuncDefPtr = std::shared_ptr<FuncDef>;
class FuncDef : public Node {
 public:
  BasicType return_btype;
  std::string name;
// #warning Have not support params yet
  BlockPtr block;
  // 存放形参列表
  std::vector<ParamPtr> params;

  FuncDef(BasicType return_btype, const char *name, BlockPtr block)
      : return_btype(return_btype), name(name), block(block) {}
  // 创建该节点的时候就是应该直接存入第一个 Param 节点
  FuncDef(BasicType return_btype, const char *name, BlockPtr block, ParamPtr param)
      : return_btype(return_btype), name(name), block(block) {
    params.push_back(param);
  }
  // 一次性获得一整个形参列表的构造函数
  FuncDef(BasicType return_btype, const char *name, BlockPtr block, const std::vector<ParamPtr> &params)
      : return_btype(return_btype), name(name), block(block), params(params) {}
  // 只有 param 的构造函数 其他为默认值 为了供给FuncFParams使用
  FuncDef(ParamPtr param) : return_btype(BasicType::Unknown), name(""), block(nullptr) {
    params.push_back(param);
  }
  // 添加一个形参
  void add_param(ParamPtr param) { params.push_back(param); }
  // 获得形参列表
  std::vector<ParamPtr> get_params() { return params; }
  std::string to_string() override {
    // 例如: FuncDef <return_btype: int, name: foo, param_count=2>
    return "FuncDef <return_btype: " +
           std::string(type_to_string(return_btype)) +
           ", name: " + name +
           ", param_count=" + std::to_string(params.size()) + ">";
  }
  std::vector<NodePtr> get_children() override {
    // 合并 params + block
    std::vector<NodePtr> children;
    children.reserve(params.size() + 1);
    for (auto &p : params) {
      children.push_back(p);
    }
    children.push_back(block);
    return children;
  }
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

// 实现 ; 或者 Exp ;
class ExpStmt;
using ExpStmtPtr = std::shared_ptr<ExpStmt>;
class ExpStmt : public Node {
 public:
  NodePtr exp;  // 这里可以是任意表达式
  ExpStmt() : exp(nullptr) {}
  ExpStmt(NodePtr exp) : exp(exp) {}
  std::string to_string() override { return "ExpStmt"; }
  std::vector<NodePtr> get_children() override { 
    return exp ? std::vector<NodePtr>{exp} : std::vector<NodePtr>();
  }
};

// 实现括号 ( Exp ) 其他的如果是 LVal 或者 Number 就直接打印了 不再规约到这里
class PrimaryExp;
using PrimaryExpPtr = std::shared_ptr<PrimaryExp>;
class PrimaryExp : public Node {
 public:
  NodePtr exp;
  PrimaryExp(NodePtr exp) : exp(exp) {}
  std::string to_string() override { return "PrimaryExp"; }
  std::vector<NodePtr> get_children() override { return {exp}; }
};



// 实现 IfStmt 表示 if / if-else
class IfStmt;
using IfStmtPtr = std::shared_ptr<IfStmt>;
class IfStmt : public Node {
 public:
  NodePtr condition;     // 条件表达式
  NodePtr then_stmt;     // if 分支
  NodePtr else_stmt;     // else 分支 可选
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

// 实现 WhileStmt 表示 while
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