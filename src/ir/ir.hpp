#ifndef IR_IR_HPP
#define IR_IR_HPP

#include <iostream>
#include <list>
#include <memory>
#include <string>
#include <sstream>

#include "common.hpp"

// 这是在IR命名空间下的Node类 和AST::Node类不同
namespace IR {

// Node 是抽象IR树所有节点的基类
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
  // "x = #k"
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
  // "x = y"
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
  // "x = y op z"
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
  // "x = op y"
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
  // "GOTO label"
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
  // "FUNCTION name:"
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
  // "x = CALL func" 或者 "CALL func"
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
  // std::string func;
  // int k;

  // Arg(const std::string &x, const std::string &func, int k)
  //     : x(x), func(func), k(k) {}

  // static ArgPtr create(const std::string &x, const std::string &func, int k) {
  //   return std::make_shared<Arg>(x, func, k);
  // }
  Arg(const std::string &x) : x(x) {}
  static ArgPtr create(const std::string &x) {
    return std::make_shared<Arg>(x);
  }
  // "ARG x"
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
  // "RETURN x" 或者 "RETURN"
  std::string to_string() const override {
    if (x.empty()) {
      return "RETURN";
    }
    return "RETURN " + x;
  }
};

// #warning Some instructions are not implemented yet
class If;
using IfPtr = std::shared_ptr<If>;
class If : public Node {
 public:
  std::string left;
  BinaryOp op;
  std::string right;
  std::string label;
  If(const std::string &l, const BinaryOp &op_, const std::string &r,
     const std::string &lbl)
      : left(l), op(op_), right(r), label(lbl) {}
  static IfPtr create(const std::string &l, const BinaryOp &op_,
                      const std::string &r, const std::string &lbl) {
    return std::make_shared<If>(l, op_, r, lbl);
  }
  std::string to_string() const override {
    // "IF left op right GOTO label"
    //  return "IF " + left + " " + op + " " + right + " GOTO " + label;
    return "IF " + left + " " + op_to_string(op) + " " + right + " GOTO " + label;
  }
};

class Param;
using ParamPtr = std::shared_ptr<Param>;
class Param : public Node {
  public:
    std::string x;
    Param(const std::string &xx) : x(xx) {}
    static std::shared_ptr<Param> create(const std::string &xx) {
      return std::make_shared<Param>(xx);
    }
    // "PARAM x"
    std::string to_string() const override {
      return "PARAM " + x;
    }
  };

// 工具函数，用于把 #k (常量) 格式化
inline std::string imm_str(int k) {
  return "#" + std::to_string(k);
}

class Dec;
using DecPtr = std::shared_ptr<Dec>;
class Dec : public Node {
  public:
    std::string x;
    int size;
    Dec(const std::string &xx, int s) : x(xx), size(s) {}
    static std::shared_ptr<Dec> create(const std::string &xx, int s) {
      return std::make_shared<Dec>(xx, s);
    }
    std::string to_string() const override {
      // "DEC x #size"
      return "DEC " + x + " " + imm_str(size);
    }
  };

class Global;
using GlobalPtr = std::shared_ptr<Global>;
class Global : public Node {
  public:
    std::string name;
    int bytes;
    // 可能有整型初始值
    std::vector<int> init_vals; // optional
    
    Global(const std::string &n, int b, const std::vector<int> &ivs)
      : name(n), bytes(b), init_vals(ivs) {}
    static std::shared_ptr<Global> create(const std::string &n, int b, const std::vector<int> &ivs) {
      return std::make_shared<Global>(n, b, ivs);
    }
    std::string to_string() const override {
      // "GLOBAL name: #bytes = #v1, #v2, ..."
      // or "GLOBAL name: #bytes"
      std::ostringstream oss;
      oss << "GLOBAL " << name << ": " << imm_str(bytes);
      if(!init_vals.empty()) {
        oss << " = ";
        for(size_t i=0; i<init_vals.size(); i++){
          if(i>0) oss << ", ";
          oss << imm_str(init_vals[i]);
        }
      }
      return oss.str();
    }
  };

// 将全局变量 label 的地址存放到 x 中
class AddressOf;
using AddressOfPtr = std::shared_ptr<AddressOf>;
class AddressOf : public Node {
  public:
    std::string dst, label;
    AddressOf(const std::string &d, const std::string &lbl)
      : dst(d), label(lbl) {}
    static std::shared_ptr<AddressOf> create(const std::string &d, const std::string &lbl){
      return std::make_shared<AddressOf>(d,lbl);
    }
    std::string to_string() const override {
      // "x = &label"
      return dst + " = &" + label;
    }
  };

//    *x = y
//    x = *y
// or offset version:  *(x + #k) = y
//    x = *(y + #k)

// store => "*x = y" or "*(x + #k) = y"
class Store;
using StorePtr = std::shared_ptr<Store>;
class Store : public Node {
  public:
    std::string addr, src;
    bool has_offset;
    int offset;
    // e.g. if has_offset==true => interpret as *(addr + #offset) = src
    Store(const std::string &a, const std::string &s)
      : addr(a), src(s), has_offset(false), offset(0) {}
    Store(const std::string &a, int off, const std::string &s)
      : addr(a), src(s), has_offset(true), offset(off) {}
    static std::shared_ptr<Store> create(const std::string &a, const std::string &s) {
      return std::make_shared<Store>(a,s);
    }
    static std::shared_ptr<Store> create(const std::string &a, int off, const std::string &s) {
      return std::make_shared<Store>(a, off, s);
    }
    std::string to_string() const override {
      if(!has_offset) {
        // "*addr = src"
        return "*" + addr + " = " + src;
      } else {
        // "*(addr + #off) = src"
        return "*(" + addr + " + " + imm_str(offset) + ") = " + src;
      }
    }
  };

// load => "x = *y" or "x = *(y + #k)"
class Load;
using LoadPtr = std::shared_ptr<Load>;
class Load : public Node {
  public:
    std::string dst, addr;
    bool has_offset;
    int offset;
  
    Load(const std::string &d, const std::string &a)
      : dst(d), addr(a), has_offset(false), offset(0) {}
    Load(const std::string &d, const std::string &a, int off)
      : dst(d), addr(a), has_offset(true), offset(off) {}
    static std::shared_ptr<Load> create(const std::string &d, const std::string &a) {
      return std::make_shared<Load>(d,a);
    }
    static std::shared_ptr<Load> create(const std::string &d, const std::string &a, int off) {
      return std::make_shared<Load>(d,a,off);
    }
    std::string to_string() const override {
      if(!has_offset) {
        return dst + " = *" + addr;
      } else {
        return dst + " = *(" + addr + " + " + imm_str(offset) + ")";
      }
    }
  };

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