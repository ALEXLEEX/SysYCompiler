#include "asm_emitter.hpp"

void ASMEmitter::emit(const Module &mod) {
#warning Global variables are not supported yet

  // 添加 Venus 的 read 和 write 系统调用
  if (use_venus) {
    output << R"(
    .text
    .globl read
read:
    li a0, 6
    ecall
    ret

    .globl write
write:
    mv a1, a0
    li a0, 1
    ecall
    ret

)";
  }

  for (const auto &func : mod.functions) {
    emit(func);
  }
}

void ASMEmitter::emit(const FunctionPtr &func) {
  int stack_size = func->temp_stack_size + func->reg_stack_size;
  reg_map = func->reg_map;  // 设置当前函数的寄存器映射

// 添加 prologue，处理 sp, ra, fp 等寄存器
#warning Not implemented: ASMEmitter::emit prologue

  // 为了方便 emit epilogue，这里忽略 exit block，也就是最后一个 block
  for (size_t i = 0; i < func->blocks.size() - 1; i++) {
    emit(func->blocks[i]);
  }

// 添加 epilogue，处理 sp, ra, fp 等寄存器
#warning Not implemented: ASMEmitter::emit epilogue
}

void ASMEmitter::emit(const BasicBlockPtr &block) {
  for (const auto &inst : block->asm_code) {
    emit(inst);
  }
}

void ASMEmitter::emit(const ASM::InstPtr &inst) {
  inst->replace_all(reg_map);
  if (type_of<ASM::Label>(inst)) {
    output << inst->to_string() << std::endl;
  } else {
    output << "    " << inst->to_string() << std::endl;
  }
}