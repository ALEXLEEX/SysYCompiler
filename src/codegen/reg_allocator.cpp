#include "reg_allocator.hpp"

#include <algorithm>
#include <map>
#include <set>
#include <vector>

#include "common.hpp"

void RegAllocator::allocate(Module &mod) {
  for (auto &func : mod.functions) {
    allocate(func);
  }
}

namespace {

using Graph = std::map<ASM::Reg, std::set<ASM::Reg>>;

std::vector<ASM::InstPtr> collect_insts(const FunctionPtr &func) {
  std::vector<ASM::InstPtr> insts;
  for (auto &block : func->blocks) {
    for (auto &inst : block->asm_code) {
      insts.push_back(inst);
    }
  }
  return insts;
}

std::set<ASM::Reg> difference(const std::set<ASM::Reg> &a,
                              const std::set<ASM::Reg> &b) {
  std::set<ASM::Reg> res;
  std::set_difference(a.begin(), a.end(), b.begin(), b.end(),
                      std::inserter(res, res.begin()));
  return res;
}

}  // namespace

void RegAllocator::allocate(FunctionPtr &func) {
  func->alloc_reg(8);  // space for ra and fp

  auto insts = collect_insts(func);
  int n = insts.size();
  std::vector<std::set<ASM::Reg>> live_in(n), live_out(n);

  bool changed = true;
  while (changed) {
    changed = false;
    for (int i = n - 1; i >= 0; --i) {
      std::set<ASM::Reg> uses = insts[i]->get_uses();
      std::set<ASM::Reg> defs = insts[i]->get_defs();
      std::set<ASM::Reg> out;
      if (i + 1 < n) out = live_in[i + 1];
      std::set<ASM::Reg> in = uses;
      auto diff = difference(out, defs);
      in.insert(diff.begin(), diff.end());
      if (in != live_in[i] || out != live_out[i]) {
        live_in[i] = in;
        live_out[i] = out;
        changed = true;
      }
    }
  }

  Graph graph;
  for (int i = 0; i < n; ++i) {
    for (auto &d : insts[i]->get_defs()) {
      if (d.is_phys()) continue;
      graph[d];
      for (auto &l : live_out[i]) {
        if (l.is_phys() || l == d) continue;
        graph[d].insert(l);
        graph[l].insert(d);
      }
    }
  }

  std::vector<ASM::Reg> phys = {ASM::Reg::t0,  ASM::Reg::t1,  ASM::Reg::t2,
                                ASM::Reg::t3,  ASM::Reg::t4,  ASM::Reg::t5,
                                ASM::Reg::t6,  ASM::Reg::s1,  ASM::Reg::s2,
                                ASM::Reg::s3,  ASM::Reg::s4,  ASM::Reg::s5,
                                ASM::Reg::s6,  ASM::Reg::s7,  ASM::Reg::s8,
                                ASM::Reg::s9,  ASM::Reg::s10, ASM::Reg::s11};

  std::vector<ASM::Reg> nodes;
  for (auto &p : graph) nodes.push_back(p.first);
  std::sort(nodes.begin(), nodes.end(), [&](const ASM::Reg &a, const ASM::Reg &b) {
    return graph[a].size() > graph[b].size();
  });

  ASM::RegMap reg_map;
  for (auto &v : nodes) {
    std::set<ASM::Reg> forbid;
    for (auto &nbor : graph[v]) {
      if (reg_map.count(nbor)) forbid.insert(reg_map[nbor]);
    }
    for (auto &r : phys) {
      if (!forbid.count(r)) {
        reg_map[v] = r;
        break;
      }
    }
    if (!reg_map.count(v) && !phys.empty()) {
      reg_map[v] = phys[0];  // fallback
    }
  }

  func->reg_map = reg_map;
}

