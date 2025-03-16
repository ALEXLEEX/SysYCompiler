#ifndef ANALYSIS_CONTROL_FLOW_HPP
#define ANALYSIS_CONTROL_FLOW_HPP

#include <memory>
#include <string>
#include <unordered_map>
#include <vector>

#include "ir/ir.hpp"

class BasicBlock;
using BasicBlockPtr = std::shared_ptr<BasicBlock>;
class BasicBlock {
 public:
  std::string label;
  IR::Code ir_code;
  std::vector<BasicBlockPtr> successors;
  std::vector<BasicBlockPtr> predecessors;

  BasicBlock(std::string label, IR::Code ir_code = {})
      : label(label), ir_code(ir_code) {}

  static BasicBlockPtr create(std::string label, IR::Code ir_code = {}) {
    return std::make_shared<BasicBlock>(label, ir_code);
  }
};

class Function;
using FunctionPtr = std::shared_ptr<Function>;
class Function {
 public:
  std::string name;
  std::vector<BasicBlockPtr> blocks;

  Function(std::string name, std::vector<BasicBlockPtr> blocks = {})
      : name(name), blocks(blocks) {}

  static FunctionPtr create(std::string name,
                            std::vector<BasicBlockPtr> blocks = {}) {
    return std::make_shared<Function>(name, blocks);
  }
};

class Module {
 public:
  std::vector<FunctionPtr> functions;
#warning Have not support global variables yet

  Module() = default;
  Module(std::vector<FunctionPtr> functions) : functions(functions) {}

  IR::Code get_ir() const;
};

#endif  // ANALYSIS_CONTROL_FLOW_HPP