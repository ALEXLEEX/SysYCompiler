#ifndef IR_IR_HPP
#define IR_IR_HPP

#include <iostream>
#include <list>
#include <memory>
#include <string>

#include "common.hpp"

namespace IR {

class Node;
using NodePtr = std::shared_ptr<Node>;
class Node {
 public:
  virtual std::string to_string() const = 0;
  virtual ~Node() = default;  // make the class polymorphic
};

class LoadImm;
using LoadImmPtr = std::shared_ptr<LoadImm>;
class LoadImm : public Node {
 public:
  std::string x;
  int k;

  LoadImm(const std::string &x, int k) : x(x), k(k) {}
  static LoadImmPtr create(const std::string &x, int k) {
    return std::make_shared<LoadImm>(x, k);
  }

  std::string to_string() const override {
    return x + " = #" + std::to_string(k);
  }
};

class Assign;
using AssignPtr = std::shared_ptr<Assign>;
class Assign : public Node {
 public:
  std::string x;
  std::string y;

  Assign(const std::string &x, const std::string &y) : x(x), y(y) {}
  static AssignPtr create(const std::string &x, const std::string &y) {
    return std::make_shared<Assign>(x, y);
  }

  std::string to_string() const override { return x + " = " + y; }
};

class Binary;
using BinaryPtr = std::shared_ptr<Binary>;
class Binary : public Node {
 public:
  std::string x;
  std::string y;
  BinaryOp op;
  std::string z;

  Binary(const std::string &x, const std::string &y, const BinaryOp &op,
         const std::string &z)
      : x(x), y(y), op(op), z(z) {}

  static BinaryPtr create(const std::string &x, const std::string &y,
                          const BinaryOp &op, const std::string &z) {
    return std::make_shared<Binary>(x, y, op, z);
  }

  std::string to_string() const override {
    return x + " = " + y + " " + op_to_string(op) + " " + z;
  }
};

class Unary;
using UnaryPtr = std::shared_ptr<Unary>;
class Unary : public Node {
 public:
  std::string x;
  BinaryOp op;
  std::string y;

  Unary(const std::string &x, const BinaryOp &op, const std::string &y)
      : x(x), op(op), y(y) {}

  static UnaryPtr create(const std::string &x, const BinaryOp &op,
                         const std::string &y) {
    return std::make_shared<Unary>(x, op, y);
  }

  std::string to_string() const override {
    return x + " = " + op_to_string(op) + " " + y;
  }
};

class Label;
using LabelPtr = std::shared_ptr<Label>;
class Label : public Node {
 public:
  std::string label;

  Label(const std::string &label) : label(label) {}

  static LabelPtr create(const std::string &label) {
    return std::make_shared<Label>(label);
  }

  std::string to_string() const override { return "LABEL " + label + ":"; }
};

class Goto;
using GotoPtr = std::shared_ptr<Goto>;
class Goto : public Node {
 public:
  std::string label;

  Goto(const std::string &label) : label(label) {}

  static GotoPtr create(const std::string &label) {
    return std::make_shared<Goto>(label);
  }

  std::string to_string() const override { return "GOTO " + label; }
};

class Function;
using FunctionPtr = std::shared_ptr<Function>;
class Function : public Node {
 public:
  std::string name;

  Function(const std::string &func) : name(func) {}

  static FunctionPtr create(const std::string &func) {
    return std::make_shared<Function>(func);
  }

  std::string to_string() const override { return "FUNCTION " + name + ":"; }
};

class Call;
using CallPtr = std::shared_ptr<Call>;
class Call : public Node {
 public:
  std::string func;
  std::string x;  // 返回值存放的位置，空字符串表示无返回值

  Call(const std::string &func) : func(func) {}
  Call(const std::string &x, const std::string &func) : func(func), x(x) {}

  static CallPtr create(const std::string &func) {
    return std::make_shared<Call>(func);
  }
  static CallPtr create(const std::string &x, const std::string &func) {
    return std::make_shared<Call>(x, func);
  }

  std::string to_string() const override {
    if (x.empty()) {
      return "CALL " + func;
    }
    return x + " = CALL " + func;
  }
};

class Arg;
using ArgPtr = std::shared_ptr<Arg>;
class Arg : public Node {
 public:
  std::string x;
  std::string func;
  int k;

  Arg(const std::string &x, const std::string &func, int k)
      : x(x), func(func), k(k) {}

  static ArgPtr create(const std::string &x, const std::string &func, int k) {
    return std::make_shared<Arg>(x, func, k);
  }

  std::string to_string() const override { return "ARG " + x; }
};

class Return;
using ReturnPtr = std::shared_ptr<Return>;
class Return : public Node {
 public:
  std::string x;  // 返回值，空字符串表示无返回值

  Return(const std::string &x = "") : x(x) {}

  static ReturnPtr create(const std::string &x = "") {
    return std::make_shared<Return>(x);
  }

  std::string to_string() const override {
    if (x.empty()) {
      return "RETURN";
    }
    return "RETURN " + x;
  }
};

#warning Some instructions are not implemented yet

using Code = std::list<NodePtr>;

}  // namespace IR

inline std::ostream &operator<<(std::ostream &os, const IR::Code &code) {
  for (const auto &node : code) {
    if (auto func = std::dynamic_pointer_cast<IR::Function>(node)) {
      os << func->to_string() << std::endl;
    } else if (auto label = std::dynamic_pointer_cast<IR::Label>(node)) {
      os << "  " << label->to_string() << std::endl;
    } else {
      os << "    " << node->to_string() << std::endl;
    }
  }
  return os;
}

#endif  // IR_IR_HPP