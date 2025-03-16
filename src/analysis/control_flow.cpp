#include "control_flow.hpp"

#include "common.hpp"

IR::Code Module::get_ir() const {
  IR::Code code;
  for (const auto &func : functions) {
    for (const auto &block : func->blocks) {
      std::copy(block->ir_code.begin(), block->ir_code.end(),
                std::back_inserter(code));
    }
  }
  return code;
}