#ifndef IR_IR_TRANSLATOR_HPP
#define IR_IR_TRANSLATOR_HPP

#include <memory>
#include <vector>

#include "ast/tree.hpp"
#include "ir/ir.hpp"

/**
 * @brief IRTranslator: 将 AST 节点翻译为 IR 代码
 */
class IRTranslator {
 public:
  // The main entry to translate any AST node (e.g. the root CompUnit)
  IR::Code translate(AST::NodePtr node);
  // For expression node translation, might store the result into 'place'
  IR::Code translateExp(AST::NodePtr node, const std::string &place = "");

  // added 辅助函数
  std::vector<int> fillupGlobalInitList(
    const std::vector<AST::InitValPtr>& sublist,
    const std::vector<int>& dims);
  IR::Code fillupLocalInitList(
    const std::vector<AST::InitValPtr> &sublist,
    const std::vector<int> &dims,
    const std::string &place = "",
    int filledCount = 0);
  // end added
  

 private:
  IR::Code translateCompUnit(AST::CompUnitPtr node);
  IR::Code translateFuncDef(AST::FuncDefPtr node);
  IR::Code translateBlock(AST::BlockPtr node);
  IR::Code translateVarDecl(AST::VarDeclPtr node);
  IR::Code translateVarDef(AST::VarDefPtr node);
  IR::Code translateAssignStmt(AST::AssignStmtPtr node);
  IR::Code translateReturnStmt(AST::ReturnStmtPtr node);

  // added
  IR::Code translateIfStmt(AST::IfStmtPtr node);
  IR::Code translateWhileStmt(AST::WhileStmtPtr node);
  IR::Code translateExpStmt(AST::ExpStmtPtr node);
  IR::Code translatePrimaryExp(AST::PrimaryExpPtr node, const std::string &place = "");
  IR::Code translateParam(AST::ParamPtr node);
  
  IR::Code translateCond(AST::NodePtr cond, const std::string &true_label, const std::string &false_label);

  // end added
  IR::Code translateLVal(AST::LValPtr node, const std::string &place = "");
  IR::Code translateBinaryExp(AST::BinaryExpPtr node,
                              const std::string &place = "");
  IR::Code translateUnaryExp(AST::UnaryExpPtr node,
                             const std::string &place = "");
  IR::Code translateFuncCall(AST::FuncCallPtr node,
                             const std::string &place = "");
  IR::Code translateIntConst(AST::IntConstPtr node,
                             const std::string &place = "");

  std::string new_temp();
  std::string new_label();
};

#endif  // IR_IR_TRANSLATOR_HPP