%{
#include <cstdio>
#include <memory>
#include <string>
#include <iostream>
#include "ast/tree.hpp"

// Bison 在遇到语法错误时会调用 yyerror
void yyerror(const char *s);

extern int yylex(void);
using namespace AST;

// Bison 语法分析结束后，最终 AST 树根会存到这个全局指针里
extern NodePtr root;

// 将普通指针转换成 std::shared_ptr<T> 的辅助函数
template <typename T>
inline std::shared_ptr<T> shared_cast(Node *ptr) {
  return std::shared_ptr<T>(static_cast<T *>(ptr));
}
%}

/** ============= union 及 token 声明 ============= **/
/** 我们在定义部分的开头使用%union{…}将所有可能的类型都包含进去。
在%token部分我们使用一对尖括号<>把需要确定属性值类型的每个词法单元所对应的类型括起
来。对于那些需要指定其属性值类型的非终结符号而言，我们使用%type加上尖括号的办法确定它
们的类型。当所有需要确定类型的符号的类型都被定下来之后，规则部分里的$$、$1等就自动地
带有了相应的类型，不再需要我们显式地为其指定类型了。 **/
// yylval 是一个 union，只能放不带构造/析构的类型或指针
%union {
    int int_val;       // 用于存整数字面量
    char *str_val;     // 用于存标识符字符串
    BinaryOp op;       // 用于存一元/二元运算符
    AST::Node *node;   // 用于存抽象语法树节点
}

// 起始产生式
%start AstRoot

// token 声明
%token ADD "+"
%token SUB "-"
%token MUL "*"
%token DIV "/"
%token MOD "%"
%token ASSIGN "="
%token SEMICOLON ";"
%token COMMA ","
%token LPAREN "("
%token RPAREN ")"
%token LBRACK "["
%token RBRACK "]"
%token LBRACE "{"
%token RBRACE "}"
%token INT "int"
%token VOID "void"
%token RETURN "return"
%token IF "if"
%token ELSE "else"
%token WHILE "while"

// 关系/逻辑
%token EQ "=="
%token NEQ "!="
%token LT "<"
%token GT ">"
%token LE "<="
%token GE ">="
%token AND "&&"
%token OR  "||"
%token NOT "!"

%token <str_val> IDENT
%token <int_val> INTCONST

// 给非终结符指定类型
%type <node> AstRoot
%type <node> CompUnit
%type <node> CompUnitItem
%type <node> Decl
%type <node> VarDecl
%type <node> VarDefs
%type <node> VarDef
%type <node> FuncDef
%type <node> FuncType
%type <node> Block
%type <node> BlockItem
%type <node> BlockItems
%type <node> Stmt
%type <node> Exp
%type <node> Cond
%type <node> LVal
%type <node> PrimaryExp
%type <node> IntConst
%type <node> UnaryExp
%type <node> FuncRParams
%type <node> MulExp
%type <node> AddExp
%type <node> RelExp
%type <node> EqExp
%type <node> LAndExp
%type <node> LOrExp

%type <op> UnaryOp  // 一元运算符 (+ - !)

%%

/** ============= 语法规则 ============= **/

/** 
 * 整个编译单元(Program) 
 * SysY约定：可以由多条声明或函数定义拼起来。 
 */
AstRoot
  : CompUnit { root = NodePtr($1); }
  ;

/** 
 * CompUnit -> CompUnitItem
 *           | CompUnit CompUnitItem
 */
CompUnit
  : CompUnitItem
  | CompUnit CompUnitItem
  ;

/** 
 * CompUnitItem -> Decl | FuncDef
 * 这样把“声明”和“函数定义”分开处理
 */
CompUnitItem
  : Decl       { $$ = $1; }
  | FuncDef    { $$ = $1; }
  ;

/** 
 * 函数定义或声明 
 */
FuncDef
  : FuncType IDENT LPAREN RPAREN Block {
      // FuncType产生式会返回一个 IntConst* (value里存int = BasicType::Int, or BasicType::Void)
      auto ft = shared_cast<IntConst>($1);
      BasicType bty = static_cast<BasicType>(ft->value);

      $$ = new FuncDef(
        bty,
        $2, // 函数名(IDENT)
        shared_cast<Block>($5)
      );
    }
    /* 如需支持带形参： 
    | FuncType IDENT LPAREN FuncFParams RPAREN Block { ... }
    */
  ;

/** 
 * 函数类型： int or void
 * 注意： 仅在函数定义时使用
 */
FuncType
  : INT {
      // 用IntConst(value=BasicType::Int)临时表示函数返回类型
      $$ = new IntConst(static_cast<int>(BasicType::Int));
    }
  | VOID {
      $$ = new IntConst(static_cast<int>(BasicType::Void));
    }
  ;

/** 
 * 声明： 只允许 int 变量
 * 例如：int a,b; 
 * 不允许 void a,b; 
 */
Decl
  : VarDecl { $$ = $1; }
  ;

/** 
 * 变量声明 
 * SysY只支持 int 作为变量声明类型 
 */
VarDecl
  : INT VarDefs SEMICOLON {
      auto vd = static_cast<VarDecl*>($2);
      vd->btype = BasicType::Int;
      $$ = vd;
    }
  ;

/** 
 * VarDefs -> VarDef
 *          | VarDefs , VarDef
 */
VarDefs
  : VarDef {
      // $1: Node*(VarDef*)
      $$ = new VarDecl(shared_cast<VarDef>($1));
    }
  | VarDefs COMMA VarDef {
      auto vard = static_cast<VarDecl*>($1);
      vard->add_def(shared_cast<VarDef>($3));
      $$ = $1;
    }
  ;

/** 
 * VarDef -> IDENT
 * (如果要支持数组定义, 在这里加  IDENT '[' INTCONST ']' {...} )
 */
VarDef
  : IDENT {
      $$ = new VarDef($1);
    }
  ;

/** 
 * 代码块 
 */
Block
  : LBRACE RBRACE {
      $$ = new Block();
    }
  | LBRACE BlockItems RBRACE {
      $$ = $2;
    }
  ;

BlockItems
  : BlockItem {
      $$ = new Block(NodePtr($1));
    }
  | BlockItems BlockItem {
      static_cast<Block*>($1)->add_stmt(NodePtr($2));
      $$ = $1;
    }
  ;

/** 
 * 代码块中可以是 声明 或 语句 
 */
BlockItem
  : Decl { $$ = $1; }
  | Stmt { $$ = $1; }
  ;

/** 
 * 语句 
 * 支持: 
 *  1) LVal '=' Exp ';'
 *  2) Exp ';'
 *  3) return [Exp] ';'
 *  4) if '(' Cond ')' Stmt [else Stmt]
 *  5) while '(' Cond ')' Stmt
 *  6) Block
 */
Stmt
  : LVal ASSIGN Exp SEMICOLON {
      $$ = new AssignStmt(shared_cast<LVal>($1), NodePtr($3));
    }
  | Exp SEMICOLON {
      $$ = $1; // 仅仅一个表达式语句
    }
  | RETURN SEMICOLON {
      $$ = new ReturnStmt(); // return;
    }
  | RETURN Exp SEMICOLON {
      $$ = new ReturnStmt(NodePtr($2)); // return expr;
    }
  | IF LPAREN Cond RPAREN Stmt {
      $$ = new IfStmt(NodePtr($3), NodePtr($5));
    }
  | IF LPAREN Cond RPAREN Stmt ELSE Stmt {
      $$ = new IfStmt(NodePtr($3), NodePtr($5), NodePtr($7));
    }
  | WHILE LPAREN Cond RPAREN Stmt {
      $$ = new WhileStmt(NodePtr($3), NodePtr($5));
    }
  | Block {
      $$ = $1;
    }
  ;

/** 
 * 条件表达式 -> 逻辑或表达式 
 */
Cond
  : LOrExp { $$ = $1; }
  ;

/** 
 * 逻辑或(LOrExp) -> 逻辑与(LAndExp) | LOrExp '||' LAndExp
 */
LOrExp
  : LAndExp { $$ = $1; }
  | LOrExp OR LAndExp {
      $$ = new BinaryExp(BinaryOp::LOr, NodePtr($1), NodePtr($3));
    }
  ;

/** 
 * 逻辑与(LAndExp) 
 */
LAndExp
  : EqExp { $$ = $1; }
  | LAndExp AND EqExp {
      $$ = new BinaryExp(BinaryOp::LAnd, NodePtr($1), NodePtr($3));
    }
  ;

/** 
 * 相等性(EqExp)
 */
EqExp
  : RelExp { $$ = $1; }
  | EqExp EQ RelExp {
      $$ = new BinaryExp(BinaryOp::EQ, NodePtr($1), NodePtr($3));
    }
  | EqExp NEQ RelExp {
      $$ = new BinaryExp(BinaryOp::NEQ, NodePtr($1), NodePtr($3));
    }
  ;

/** 
 * 关系(RelExp) 
 */
RelExp
  : AddExp { $$ = $1; }
  | RelExp LT AddExp {
      $$ = new BinaryExp(BinaryOp::LT, NodePtr($1), NodePtr($3));
    }
  | RelExp GT AddExp {
      $$ = new BinaryExp(BinaryOp::GT, NodePtr($1), NodePtr($3));
    }
  | RelExp LE AddExp {
      $$ = new BinaryExp(BinaryOp::LE, NodePtr($1), NodePtr($3));
    }
  | RelExp GE AddExp {
      $$ = new BinaryExp(BinaryOp::GE, NodePtr($1), NodePtr($3));
    }
  ;

/** 
 * Exp -> AddExp 
 */
Exp
  : AddExp { $$ = $1; }
  ;

/** 
 * 加法(AddExp) -> 乘法(MulExp) | AddExp '+' MulExp | ...
 */
AddExp
  : MulExp { $$ = $1; }
  | AddExp ADD MulExp {
      $$ = new BinaryExp(BinaryOp::Add, NodePtr($1), NodePtr($3));
    }
  | AddExp SUB MulExp {
      $$ = new BinaryExp(BinaryOp::Sub, NodePtr($1), NodePtr($3));
    }
  ;

/** 
 * 乘法(MulExp) -> 一元(UnaryExp) | MulExp '*' UnaryExp | ...
 */
MulExp
  : UnaryExp { $$ = $1; }
  | MulExp MUL UnaryExp {
      $$ = new BinaryExp(BinaryOp::Mul, NodePtr($1), NodePtr($3));
    }
  | MulExp DIV UnaryExp {
      $$ = new BinaryExp(BinaryOp::Div, NodePtr($1), NodePtr($3));
    }
  | MulExp MOD UnaryExp {
      $$ = new BinaryExp(BinaryOp::Mod, NodePtr($1), NodePtr($3));
    }
  ;

/** 
 * UnaryExp -> PrimaryExp 
 *           | IDENT '(' [FuncRParams] ')' 
 *           | UnaryOp UnaryExp
 */
UnaryExp
  : PrimaryExp { $$ = $1; }
  | IDENT LPAREN RPAREN {
      $$ = new FuncCall($1); // 无参函数调用
    }
  | IDENT LPAREN FuncRParams RPAREN {
      static_cast<FuncCall*>($3)->name = $1;
      $$ = $3; // 有参函数调用
    }
  | UnaryOp UnaryExp {
      $$ = new UnaryExp($1, NodePtr($2));
    }
  ;

/** 
 * 一元运算符 
 */
UnaryOp
  : ADD { $$ = BinaryOp::Add; }
  | SUB { $$ = BinaryOp::Sub; }
  | NOT { $$ = BinaryOp::Not; }
  ;

/** 
 * 函数实参(可变长) 
 * FuncRParams -> Exp | FuncRParams ',' Exp
 */
FuncRParams
  : Exp {
      $$ = new FuncCall(NodePtr($1));
    }
  | FuncRParams COMMA Exp {
      static_cast<FuncCall*>($1)->add_arg(NodePtr($3));
      $$ = $1;
    }
  ;

/** 
 * 基础表达式 -> '(' Exp ')' | LVal | IntConst
 */
PrimaryExp
  : LPAREN Exp RPAREN {
      $$ = $2;
    }
  | LVal {
      $$ = $1;
    }
  | IntConst {
      $$ = $1;
    }
  ;

/** 
 * IntConst -> INTCONST
 */
IntConst
  : INTCONST {
      $$ = new IntConst($1);
    }
  ;

/** 
 * LVal -> IDENT 
 * (若要支持数组LVal -> IDENT '[' Exp ']' [...]) 
 */
LVal
  : IDENT {
      $$ = new LVal($1);
    }
  ;

%%

// 语法错误处理
void yyerror(const char *s) {
    std::cerr << "Parse error: " << s << std::endl;
}
