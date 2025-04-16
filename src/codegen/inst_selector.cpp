#include "inst_selector.hpp"

#include <cassert>
#include <memory>

#include "codegen/asm_emitter.hpp"

void InstSelector::select(Module &mod) {
  for (auto &func : mod.functions) {
    select(func);
  }
}

void InstSelector::select(FunctionPtr &func) {
  // 设置当前函数
  // 如果是 DEC，需要调用当前函数中的 alloc_temp，在栈上分配空间
  current_func = func;
  for (auto &block : func->blocks) {
    block->asm_code = select(block->ir_code);
  }
}

ASM::Code InstSelector::select(const IR::Code &ir_code) {
  ASM::Code asm_code;
  for (const auto &node : ir_code) {
    auto code = select(node);
    asm_code.insert(asm_code.end(), code.begin(), code.end());
  }
  return asm_code;
}

ASM::Code InstSelector::select(const IR::NodePtr &node) {
#define SELECT_NODE(type)                                   \
  if (auto p = std::dynamic_pointer_cast<IR::type>(node)) { \
    return select##type(p);                                 \
  }

  // 对于每种不同类型的 IR 节点，调用相应的 select 函数
  // 如果你添加了新的 IR 节点类型，记得在这里添加对应的 select 函数
  SELECT_NODE(LoadImm)
  SELECT_NODE(Assign)
  SELECT_NODE(Binary)
  SELECT_NODE(Unary)
  SELECT_NODE(Label)
  SELECT_NODE(Goto)
  SELECT_NODE(Function)
  SELECT_NODE(Call)
  SELECT_NODE(Arg)
  SELECT_NODE(Return)

  // added
  SELECT_NODE(Load)
  SELECT_NODE(Store)
  SELECT_NODE(If)
  SELECT_NODE(Param)
  SELECT_NODE(Dec)
  SELECT_NODE(AddressOf)
  // end added
// #warning Add more IR node types if needed

  assert(false && "Unknown IR node type");
}

ASM::Code InstSelector::selectLoadImm(const IR::LoadImmPtr &node) {
  ASM::Code code;
  // a = b + #t	-> addi reg(a), reg(b), t
  // code.push_back(ASM::ArithImm::create(ASM::Reg(node->x), ASM::Reg::zero, node->k, ASM::ArithImm::Op::Addi));
  // 需要判断立即数大小是否超过 12 位 超过的话要特殊处理
  if (node->k > 2047 || node->k < -2048) {
    code.push_back(ASM::Lim::create(ASM::Reg::t0, node->k));
    code.push_back(ASM::Arith::create(ASM::Reg::t0, ASM::Reg::zero, ASM::Reg::t0, ASM::Arith::Op::Add));
  } else {
    code.push_back(ASM::ArithImm::create(ASM::Reg::t0, ASM::Reg::zero, node->k, ASM::ArithImm::Op::Addi));
  }
  
  // int offset_rd = current_func->alloc_temp();
  int offset_rd = get_temp_offset(node->x);
  // code.push_back(ASM::Store::create(ASM::Reg::sp, ASM::Reg(node->x), offset_rd));
  code.push_back(ASM::Store::create(ASM::Reg::sp, ASM::Reg::t0, offset_rd));
  // current_func->temp_var_offset_map[node->x] = offset_rd;
// #warning Not implemented: InstSelector::selectLoadImm
  return code;
}

ASM::Code InstSelector::selectAssign(const IR::AssignPtr &node) {
  ASM::Code code;
  // a = b	-> mv reg(a), reg(b)

  // int offset_rs = current_func->temp_var_offset_map[node->y];
  int offset_rs = get_temp_offset(node->y);
  // code.push_back(ASM::Load::create(ASM::Reg(node->y), ASM::Reg::sp, offset_rs));
  code.push_back(ASM::Load::create(ASM::Reg::t1, ASM::Reg::sp, offset_rs));

  // code.push_back(ASM::Mv::create(ASM::Reg(node->x), ASM::Reg(node->y)));
  if (node->x == "a0") {
    code.push_back(ASM::Mv::create(ASM::Reg::a0, ASM::Reg::t1));
  } else {
    code.push_back(ASM::Mv::create(ASM::Reg::t0, ASM::Reg::t1));
    // int offset_rd = current_func->alloc_temp();
    int offset_rd = get_temp_offset(node->x);
    // code.push_back(ASM::Store::create(ASM::Reg::sp, ASM::Reg(node->x), offset_rd));
    code.push_back(ASM::Store::create(ASM::Reg::sp, ASM::Reg::t0, offset_rd));
    // current_func->temp_var_offset_map[node->x] = offset_rd;
  }

  return code;
}

ASM::Code InstSelector::selectBinary(const IR::BinaryPtr &node) {
  ASM::Code code;
  // a = b + c	-> add reg(a), reg(b), reg(c)
  // a = b - c	-> sub reg(a), reg(b), reg(c)
  // a = b * c	-> mul reg(a), reg(b), reg(c)
  // a = b / c	-> div reg(a), reg(b), reg(c)
  // a = b % c	-> rem reg(a), reg(b), reg(c)

  // int offset_rs1 = current_func->temp_var_offset_map[node->y];
  // int offset_rs2 = current_func->temp_var_offset_map[node->z];
  int offset_rs1 = get_temp_offset(node->y);
  int offset_rs2 = get_temp_offset(node->z);
  // code.push_back(ASM::Load::create(ASM::Reg(node->y), ASM::Reg::sp, offset_rs1));
  code.push_back(ASM::Load::create(ASM::Reg::t1, ASM::Reg::sp, offset_rs1));
  code.push_back(ASM::Load::create(ASM::Reg::t2, ASM::Reg::sp, offset_rs2));

  // 一旦replace reg之后 这条指令
  // code.push_back(ASM::Arith::create(ASM::Reg(node->x), ASM::Reg(node->y), ASM::Reg::t4, ASM::Arith::Op(node->op)));
  code.push_back(ASM::Arith::create(ASM::Reg::t0, ASM::Reg::t1, ASM::Reg::t2, ASM::Arith::Op(node->op)));

  // int offset_rd = current_func->alloc_temp();
  int offset_rd = get_temp_offset(node->x);
  // code.push_back(ASM::Store::create(ASM::Reg::sp, ASM::Reg(node->x), offset_rd));
  code.push_back(ASM::Store::create(ASM::Reg::sp, ASM::Reg::t0, offset_rd));
  // current_func->temp_var_offset_map[node->x] = offset_rd;

// #warning Not implemented: InstSelector::selectBinary
  return code;
}

ASM::Code InstSelector::selectUnary(const IR::UnaryPtr &node) {
  ASM::Code code;
  // a = -b	-> sub reg(a), zero, reg(b)

  // int offset_rs = current_func->temp_var_offset_map[node->y];
  int offset_rs = get_temp_offset(node->y);
  // code.push_back(ASM::Load::create(ASM::Reg(node->y), ASM::Reg::sp, offset_rs));
  code.push_back(ASM::Load::create(ASM::Reg::t1, ASM::Reg::sp, offset_rs));

  // code.push_back(ASM::Arith::create(ASM::Reg(node->x), ASM::Reg::zero, ASM::Reg(node->y), ASM::Arith::Op(node->op)));
  code.push_back(ASM::Arith::create(ASM::Reg::t0, ASM::Reg::zero, ASM::Reg::t1, ASM::Arith::Op(node->op)));

  // int offset_rd = current_func->alloc_temp();
  int offset_rd = get_temp_offset(node->x);
  // code.push_back(ASM::Store::create(ASM::Reg::sp, ASM::Reg(node->x), offset_rd));
  code.push_back(ASM::Store::create(ASM::Reg::sp, ASM::Reg::t0, offset_rd));
  // current_func->temp_var_offset_map[node->x] = offset_rd;

// #warning Not implemented: InstSelector::selectUnary
  return code;
}

ASM::Code InstSelector::selectLabel(const IR::LabelPtr &node) {
  ASM::Code code;
  // LABEL label:	-> label:

  code.push_back(ASM::Label::create(node->label));
// #warning Not implemented: InstSelector::selectLabel
  return code;
}

ASM::Code InstSelector::selectGoto(const IR::GotoPtr &node) {
  ASM::Code code;
  // GOTO label	-> j label
  code.push_back(ASM::Jump::create(node->label));
// #warning Not implemented: InstSelector::selectGoto
  return code;
}

ASM::Code InstSelector::selectFunction(const IR::FunctionPtr &node) {
  ASM::Code code;
  // FUNCTION func:	-> func:
  // 设置一个函数名的 label
  // code.push_back(ASM::Label::create(node->name));

  param_count = 0;
// #warning Not implemented: InstSelector::selectFunction
  return code;
}

ASM::Code InstSelector::selectCall(const IR::CallPtr &node) {
  ASM::Code code;
  // CALL func	-> call func
  // x = CALL func	-> call func; mv reg(x), a0
  code.push_back(ASM::Call::create(node->func));
  if (!node->x.empty()) {
    // code.push_back(ASM::Mv::create(ASM::Reg(node->x), ASM::Reg::a0));
    code.push_back(ASM::Mv::create(ASM::Reg::t0, ASM::Reg::a0));

    // int offset_rd = current_func->alloc_temp();
    int offset_rd = get_temp_offset(node->x);
    // code.push_back(ASM::Store::create(ASM::Reg::sp, ASM::Reg(node->x), offset_rd));
    code.push_back(ASM::Store::create(ASM::Reg::sp, ASM::Reg::t0, offset_rd));
    // current_func->temp_var_offset_map[node->x] = offset_rd;
  }
  arg_count = 0;
// #warning Not implemented: InstSelector::selectCall
  return code;
}

ASM::Code InstSelector::selectArg(const IR::ArgPtr &node) {
  ASM::Code code;
  // ARG x	-> mv ak, reg(x)
  // k is the index of the argument

  // int offset_rs = current_func->temp_var_offset_map[node->x];
  int offset_rs = get_temp_offset(node->x);
  // code.push_back(ASM::Load::create(ASM::Reg(node->x), ASM::Reg::sp, offset_rs));
  code.push_back(ASM::Load::create(ASM::Reg::t1, ASM::Reg::sp, offset_rs));

  if (arg_count < 8) {
    std::string reg_name = "a" + std::to_string(arg_count);
    // code.push_back(ASM::Mv::create(ASM::Reg(reg_name), ASM::Reg(node->x)));
    code.push_back(ASM::Mv::create(ASM::Reg(reg_name), ASM::Reg::t1));
  } else {
    // 如果参数超过了 8 个，使用栈来传递参数
    // 栈上面已经预留了从 sp 开始的 30 个字节的空间 依次存储即可
    code.push_back(ASM::Store::create(ASM::Reg::sp, ASM::Reg::t1, 4 * (arg_count - 8)));
  }
  arg_count++;
// #warning Not implemented: InstSelector::selectArg
  return code;
}

ASM::Code InstSelector::selectReturn(const IR::ReturnPtr &node) {
  ASM::Code code;
  // 已经在 cfg builder 中统一为一个 exit call
  // 因此只有 exit block 里有 return 语句
  // 在 asm emitter 中对每个函数处理时
  // 会忽略 exit block 并添加 epilogue
  // 因此这里不需要处理

// #warning Not implemented: InstSelector::selectReturn
  return code;
}

ASM::Code InstSelector::selectLoad(const IR::LoadPtr &node) {
  ASM::Code code;
  // a = *b	-> lw reg(a), 0(reg(b))
  // a = *(b + #k)	-> lw reg(a), k(reg(b))

  // int offset_rs = current_func->temp_var_offset_map[node->addr];
  int offset_rs = get_temp_offset(node->addr);
  // code.push_back(ASM::Load::create(ASM::Reg(node->addr), ASM::Reg::sp, offset_rs));
  code.push_back(ASM::Load::create(ASM::Reg::t1, ASM::Reg::sp, offset_rs));

  if (node->has_offset) {
    // code.push_back(ASM::Load::create(ASM::Reg(node->dst), ASM::Reg(node->addr), node->offset));
    code.push_back(ASM::Load::create(ASM::Reg::t0, ASM::Reg::t1, node->offset));
  } else {
    // code.push_back(ASM::Load::create(ASM::Reg(node->dst), ASM::Reg(node->addr), 0));
    code.push_back(ASM::Load::create(ASM::Reg::t0, ASM::Reg::t1, 0));
  }

  // int offset_rd = current_func->alloc_temp();
  int offset_rd = get_temp_offset(node->dst);
  // code.push_back(ASM::Store::create(ASM::Reg::sp, ASM::Reg(node->dst), offset_rd));
  code.push_back(ASM::Store::create(ASM::Reg::sp, ASM::Reg::t0, offset_rd));
  // current_func->temp_var_offset_map[node->dst] = offset_rd;

  return code;
}

ASM::Code InstSelector::selectStore(const IR::StorePtr &node) {
  ASM::Code code;
  // *a = b	-> sw reg(b), 0(reg(a))
  // *(a + #k) = b	-> sw reg(b), k(reg(a))

  // int offset_rs = current_func->temp_var_offset_map[node->addr];
  int offset_rs = get_temp_offset(node->addr);
  // code.push_back(ASM::Load::create(ASM::Reg(node->addr), ASM::Reg::sp, offset_rs));
  code.push_back(ASM::Load::create(ASM::Reg::t1, ASM::Reg::sp, offset_rs));
  // int offset_rd = current_func->temp_var_offset_map[node->src];
  int offset_rd = get_temp_offset(node->src);
  // code.push_back(ASM::Load::create(ASM::Reg(node->src), ASM::Reg::sp, offset_rd));
  code.push_back(ASM::Load::create(ASM::Reg::t2, ASM::Reg::sp, offset_rd));

  if (node->has_offset) {
    // code.push_back(ASM::Store::create(ASM::Reg(node->addr), ASM::Reg(node->src), node->offset));
    code.push_back(ASM::Store::create(ASM::Reg::t1, ASM::Reg::t2, node->offset));
  } else {
    // code.push_back(ASM::Store::create(ASM::Reg(node->addr), ASM::Reg(node->src), 0));
    code.push_back(ASM::Store::create(ASM::Reg::t1, ASM::Reg::t2, 0));
  }
  return code;
}

ASM::Code InstSelector::selectIf(const IR::IfPtr &node) {
  ASM::Code code;
  // if (left == right) goto label	-> beq reg(left), reg(right), label
  // if (left != right) goto label	-> bne reg(left), reg(right), label
  // if (left < right) goto label	-> blt reg(left), reg(right), label
  // if (left > right) goto label	-> bgt reg(left), reg(right), label
  // if (left <= right) goto label	-> ble reg(left), reg(right), label
  // if (left >= right) goto label	-> bge reg(left), reg(right), label

  // int offset_rs1 = current_func->temp_var_offset_map[node->left];
  // int offset_rs2 = current_func->temp_var_offset_map[node->right];
  int offset_rs1 = get_temp_offset(node->left);
  int offset_rs2 = get_temp_offset(node->right);
  // code.push_back(ASM::Load::create(ASM::Reg(node->left), ASM::Reg::sp, offset_rs1));
  code.push_back(ASM::Load::create(ASM::Reg::t1, ASM::Reg::sp, offset_rs1));
  // code.push_back(ASM::Load::create(ASM::Reg(node->right), ASM::Reg::sp, offset_rs2));
  code.push_back(ASM::Load::create(ASM::Reg::t2, ASM::Reg::sp, offset_rs2));

  // code.push_back(ASM::Branch::create(ASM::Reg(node->left), ASM::Reg(node->right), node->label, ASM::Branch::Op(node->op)));
  code.push_back(ASM::Branch::create(ASM::Reg::t1, ASM::Reg::t2, node->label, ASM::Branch::Op(node->op)));
  return code;
}

ASM::Code InstSelector::selectParam(const IR::ParamPtr &node) {
  ASM::Code code;
  // PARAM x	-> alloc_temp(x) 获取参数
  
  if (param_count < 8) {
    std::string reg_name = "a" + std::to_string(param_count);
    // code.push_back(ASM::Mv::create(ASM::Reg(node->x), ASM::Reg(reg_name)));
    code.push_back(ASM::Mv::create(ASM::Reg::t0, ASM::Reg(reg_name)));
  } else {
    // 如果参数超过了 8 个，使用栈来传递参数
    // 通过 fp 向之前函数的栈帧获取参数
    code.push_back(ASM::Load::create(ASM::Reg::t0, ASM::Reg::fp, 4 * (param_count - 7)));
  }

  // int offset = current_func->alloc_temp();
  int offset = get_temp_offset(node->x);
  // code.push_back(ASM::Store::create(ASM::Reg::sp, ASM::Reg(node->x), offset));
  code.push_back(ASM::Store::create(ASM::Reg::sp, ASM::Reg::t0, offset));
  // current_func->temp_var_offset_map[node->x] = offset;
  
  param_count++;
  return code;
}

ASM::Code InstSelector::selectDec(const IR::DecPtr &node) {
  ASM::Code code;
  // DEC x	-> alloc_temp(x) 应该存下 x 的地址 x = sp + offset
  int offset_array = current_func->alloc_temp(node->size);
  // int offset_base = current_func->alloc_temp(); // 基地址具体是多少 存放在 sp + offset_base
  int offset_base = get_temp_offset(node->x);
  // current_func->temp_var_offset_map[node->x] = offset_base;
  // code.push_back(ASM::ArithImm::create(ASM::Reg::t3, ASM::Reg::sp, offset_array, ASM::ArithImm::Op::Addi));
  code.push_back(ASM::ArithImm::create(ASM::Reg::t0, ASM::Reg::sp, offset_array, ASM::ArithImm::Op::Addi));
  // code.push_back(ASM::Store::create(ASM::Reg::sp, ASM::Reg::t3, offset_base));
  code.push_back(ASM::Store::create(ASM::Reg::sp, ASM::Reg::t0, offset_base));
  
  return code;
}

ASM::Code InstSelector::selectAddressOf(const IR::AddressOfPtr &node) {
  ASM::Code code;
  // a = &b	-> la reg(a), b
  // code.push_back(ASM::La::create(ASM::Reg(node->dst), node->label));
  code.push_back(ASM::La::create(ASM::Reg::t0, node->label));

  // int offset_rd = current_func->alloc_temp();
  int offset_rd = get_temp_offset(node->dst);
  // code.push_back(ASM::Store::create(ASM::Reg::sp, ASM::Reg(node->dst), offset_rd));
  code.push_back(ASM::Store::create(ASM::Reg::sp, ASM::Reg::t0, offset_rd));
  // current_func->temp_var_offset_map[node->dst] = offset_rd;

  return code;
}