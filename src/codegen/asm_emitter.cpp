#include "asm_emitter.hpp"

void ASMEmitter::emit(const Module &mod) {
// #warning Global variables are not supported yet
  // 添加 .data 段
  output << ".data" << std::endl;
  for (const auto &global : mod.global_ir) {
    if (auto global_var = std::dynamic_pointer_cast<IR::Global>(global)) {
      // 类似这样
      //array:
      // .word   1
      // .word   2
      // .word   3
      // .word   4
      output << global_var->name << ":" << std::endl;
      for (const auto &val : global_var->init_vals) {
        output << "    .word " << val << std::endl;
      }
    }
  }
  output << std::endl;

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

  output << "    .section .text" << std::endl;

  for (const auto &func : mod.functions) {
    emit(func);
  }
}

void ASMEmitter::emit(const FunctionPtr &func) {
  int stack_size = func->temp_stack_size + func->reg_stack_size;
  reg_map = func->reg_map;  // 设置当前函数的寄存器映射

// 添加 prologue，处理 sp, ra, fp 等寄存器
  output << ".globl " << func->name << std::endl;
  output << func->name << ":" << std::endl;
  
  // TODO: 这里也要考虑是不是超出 addi sw lw 的立即数限制
  if (stack_size > 2047) {
    // t4 = stack_size
    output << "    li t4, " << stack_size << std::endl;
    // t5 = -stack_size
    output << "    sub t5, zero, t4" << std::endl;
    // sp = sp - stack_size
    output << "    add sp, sp, t5" << std::endl;
    // t6 = sp + stack_size
    output << "    add t6, sp, t4" << std::endl;
    output << "    sw ra, -4(t6)" << std::endl;
    output << "    sw fp, -8(t6)" << std::endl;
    output << "    add fp, t6, -4" << std::endl;
  } else {
    output << "    addi sp, sp, -" << stack_size << std::endl;
    output << "    sw ra, " << (stack_size - 4) << "(sp)" << std::endl;
    output << "    sw fp, " << (stack_size - 8) << "(sp)" << std::endl;
    output << "    addi fp, sp, " << (stack_size - 4) << std::endl;
  }
  
  // output << "    addi sp, sp, -" << stack_size << std::endl;
  // output << "    sw ra, " << (stack_size - 4) << "(sp)" << std::endl;
  // output << "    sw fp, " << (stack_size - 8) << "(sp)" << std::endl;
  // output << "    addi fp, sp, " << (stack_size - 4) << std::endl;

// #warning Not implemented: ASMEmitter::emit prologue

  // 为了方便 emit epilogue，这里忽略 exit block，也就是最后一个 block
  for (size_t i = 0; i < func->blocks.size() - 1; i++) {
    emit(func->blocks[i]);
  }

// 添加 epilogue，处理 sp, ra, fp 等寄存器
  output << func->blocks.back()->label << ":" << std::endl;
  if (stack_size > 2047) {
    // t4 = stack_size
    output << "    li t4, " << stack_size << std::endl;
    output << "    add t5, sp, t4" << std::endl;
    output << "    lw ra, -4(t5)" << std::endl;
    output << "    lw fp, -8(t5)" << std::endl;
    output << "    add sp, sp, t4" << std::endl;
  } else {
    output << "    lw ra, " << (stack_size - 4) << "(sp)" << std::endl;
    output << "    lw fp, " << (stack_size - 8) << "(sp)" << std::endl;
    output << "    addi sp, sp, " << stack_size << std::endl;
  }
  // output << "    lw ra, " << (stack_size - 4) << "(sp)" << std::endl;
  // output << "    lw fp, " << (stack_size - 8) << "(sp)" << std::endl;
  // output << "    addi sp, sp, " << stack_size << std::endl;
  
  output << "    ret" << std::endl;
// #warning Not implemented: ASMEmitter::emit epilogue
}

void ASMEmitter::emit(const BasicBlockPtr &block) {
  for (const auto &inst : block->asm_code) {
    emit(inst);
  }
}

void ASMEmitter::emit(const ASM::InstPtr &inst) {
  // inst->replace_all(reg_map);
  if (type_of<ASM::Label>(inst)) {
    output << inst->to_string() << std::endl;
  } else {
    output << "    " << inst->to_string() << std::endl;
  }
}