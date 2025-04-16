#include "cfg_builder.hpp"

#include <unordered_map>

Module CFGBuilder::build(IR::Code code) {
  Module mod;
  IR::Code current_func;

#warning Global variable is not supported yet

  // 按函数分割IR代码
  for (const auto &inst : code) {
    if (auto global = std::dynamic_pointer_cast<IR::Global>(inst)) {
      mod.global_ir.push_back(inst);
      continue;
    }
    // 这里处理的是上一个函数 就是说检测到新的IR::Function的时候
    // 就把之前的所有压入current_func的东西放入build_single_func
    if (auto func = std::dynamic_pointer_cast<IR::Function>(inst)) {
      if (!current_func.empty()) {
        mod.functions.push_back(build_single_func(current_func));
        current_func.clear();
      }
    }
    current_func.push_back(inst);
  }

  if (!current_func.empty()) {
    mod.functions.push_back(build_single_func(current_func));
  }

  return mod;
}

FunctionPtr CFGBuilder::build_single_func(IR::Code code) {
  std::vector<BasicBlockPtr> blocks;
  std::unordered_map<std::string, BasicBlockPtr> label_to_block;
  std::string func_name;
  bool ret_void = true;  // 函数返回值是否为 void

  bool has_ret = false;

  // 统一为一个 ret block 处理返回
  // 这里没有完成基本块划分，而是把所有函数体内的代码放在一个基本块中
  // 一个函数两个基本块 一个主块 一个返回块 没有用到BasicBlock中的前驱和后继
  // 先跳过此处函数内部基本块划分 先完成目标代码生成
  // 如果你想要更细粒度的基本块，可以参考实验文档修改这里的逻辑

// #warning Only one block is created for the whole function body now

  BasicBlockPtr current_block = BasicBlock::create("entry");
  for (const auto &inst : code) {
    if (auto func = std::dynamic_pointer_cast<IR::Function>(inst)) {
      func_name = func->name;
      current_block->label = func_name + ".entry";
      current_block->ir_code.push_back(inst);
    } else if (auto ret = std::dynamic_pointer_cast<IR::Return>(inst)) {
      if (ret->x.empty()) {
        current_block->ir_code.push_back(IR::Goto::create(func_name + ".ret"));
      } else {
        current_block->ir_code.push_back(IR::Assign::create("a0", ret->x));
        current_block->ir_code.push_back(IR::Goto::create(func_name + ".ret"));
        ret_void = false;
      }
      has_ret = true;
    } else {
      current_block->ir_code.push_back(inst);
    }
  }
  
  // 为了解决有些 void 函数没有 return; 语句
  if (has_ret == false) {
    current_block->ir_code.push_back(IR::Goto::create(func_name + ".ret"));
    ret_void = true;
  }

  blocks.push_back(current_block);
  label_to_block[current_block->label] = current_block;

  // 添加 exit block，统一处理函数退出
  auto exit_block = BasicBlock::create(func_name + ".ret");
  exit_block->ir_code.push_back(IR::Label::create(func_name + ".ret"));
  if (!ret_void) {
    exit_block->ir_code.push_back(IR::Return::create("a0"));
  } else {
    exit_block->ir_code.push_back(IR::Return::create());
  }
  blocks.push_back(exit_block);
  label_to_block[func_name + ".ret"] = exit_block;

  return Function::create(func_name, blocks);
}

std::string CFGBuilder::new_label() {
  static int label_count = 0;
  return "LN" + std::to_string(label_count++);
}
