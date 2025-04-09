#include "reg_allocator.hpp"

#include <set>

void RegAllocator::allocate(Module &mod) {
  for (auto &func : mod.functions) {
    allocate(func);
  }
}

void RegAllocator::allocate(FunctionPtr &func) {
  func->alloc_reg(8);   // allocate space for return address and previous fp
  ASM::RegMap reg_map;  // virtual register to physical register
  std::set<ASM::Reg> available_regs = {
      ASM::Reg::t0,  ASM::Reg::t1, ASM::Reg::t2, ASM::Reg::t3, ASM::Reg::t4,
      ASM::Reg::t5,  ASM::Reg::t6, ASM::Reg::s2, ASM::Reg::s3, ASM::Reg::s4,
      ASM::Reg::s5,  ASM::Reg::s6, ASM::Reg::s7, ASM::Reg::s8, ASM::Reg::s9,
      ASM::Reg::s10, ASM::Reg::s11};
  // 在这里实现寄存器分配算法，将结果保存在 reg_map 中
  // tips: 对于朴素的仅用到三个寄存器的算法，可能用不到 reg_map

#warning Not implemented: RegAllocator::allocate

  func->reg_map = reg_map;
}