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

  // 我的思路 采用最简单的寄存器分配算法
  // 只用 t0 t1 t2 分别对应 rd rs1 rs2
  // 对每一个临时变量都要存在栈上
  // 需要建立一个临时变量和他偏移量的对应表
  // 逐条检查func->blocks->asm_code
  // 对于rs1 rs2寄存器 从对应表获取偏移量 把load指令插入这条asm_code的前面 先把值load进t1 t2
  // 对于rd寄存器 就新分配一个位置 获取他的偏移量 然后把store指令插入这条asm_code的后面


  // for (auto &block : func->blocks) {
  //   for (auto &inst : block->asm_code) {
  //     auto uses = inst->get_uses();
  //     auto defs = inst->get_defs();
  //     auto it = uses.begin();
  //     for (int i = 0; i < uses.size(); i++) {
  //       if (i == 0 && it->is_phys() == false) {
  //         // reg_map[it->name] = ASM::Reg::t1;
  //         reg_map[*it] = ASM::Reg::t1;
  //       } else if (i == 1 && it->is_phys() == false) {
  //         // reg_map[it->name] = ASM::Reg::t2;
  //         reg_map[*it] = ASM::Reg::t2;
  //       }
  //       std::cerr << "reg_map[" << it->name << "] = tx" << std::endl;
  //       it++;
  //     }
  //     it = defs.begin();
  //     for (int i = 0; i < defs.size(); i++) {
  //       if (i == 0 && it->is_phys() == false) {
  //         // reg_map[it->name] = ASM::Reg::t0;
  //         std::cerr << "reg_map[" << it->name << "] = t0" << std::endl;
  //         reg_map[*it] = ASM::Reg::t0;
  //       }
  //       it++;
  //     }
  //   }
  // }
// #warning Not implemented: RegAllocator::allocate
  func->reg_map = reg_map;
}