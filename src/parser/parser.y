%{
#include <cstdio>
#include <memory>
#include <string>
#include <iostream>
#include "ast/tree.hpp"
#define YYDEBUG 1
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
%type <node> Decl
%type <node> VarDecl
%type <node> VarDefs
%type <node> VarDef
%type <node> InitVal
%type <node> InitValList
%type <node> ArrayDims
%type <node> ArrayDim
%type <node> FuncDef
%type <node> FuncFParams
%type <node> FuncFParam
%type <node> Block
%type <node> BlockItem
%type <node> BlockItems
%type <node> Stmt
%type <node> Exp
%type <node> Cond
%type <node> LVal
%type <node> LValArrayDims
%type <node> LValArrayDim
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

%nonassoc LOWER_THAN_ELSE
%nonassoc ELSE

%%

// 由于我们在后续代码中所有指针都是 std::shared_ptr
// 而在 union 中没法直接定义 std::shared_ptr
// 所以我们在这里需要将普通的指针转换成 std::shared_ptr
AstRoot : CompUnit { root = NodePtr($1); }
    ;

CompUnit : FuncDef { $$ = new CompUnit(shared_cast<FuncDef>($1)); }
    | CompUnit FuncDef { static_cast<CompUnit*>($1)->add_unit(shared_cast<FuncDef>($2)); $$ = $1; }
    | VarDecl { $$ = new CompUnit(shared_cast<VarDecl>($1)); }
    | CompUnit VarDecl { static_cast<CompUnit*>($1)->add_unit(shared_cast<VarDecl>($2)); $$ = $1; }
    ;

Decl : VarDecl { $$ = $1; }
    ;

// 由于 union 中的类型不能是 std::shared_ptr
// 只是普通的指针，所以我们需要通过 static_cast 来转换类型
// 才能访问到对应的成员和函数
VarDecl : "int" VarDefs ";" { static_cast<VarDecl *>($2)->btype = BasicType::Int; $$ = $2; }
    ;

VarDefs : VarDef { $$ = new VarDecl(shared_cast<VarDef>($1)); }
    | VarDefs "," VarDef { static_cast<VarDecl*>($1)->add_def(shared_cast<VarDef>($3)); $$ = $1; }
    ;

VarDef : IDENT { $$ = new VarDef($1); }
    | IDENT "=" InitVal { $$ = new VarDef($1, shared_cast<InitVal>($3)); }
    | IDENT ArrayDims { static_cast<VarDef*>($2)->set_ident($1); $$ = $2; }
    | IDENT ArrayDims "=" InitVal {
        // 设置名字
        static_cast<VarDef*>($2)->set_ident($1);
        // 添加初值
        static_cast<VarDef*>($2)->add_initVal(shared_cast<InitVal>($4));
        $$ = $2;
      }
    ;

ArrayDims : ArrayDim { $$ = new VarDef((shared_cast<IntConst>($1))->get_value()); }
    | ArrayDim ArrayDims { static_cast<VarDef*>($2)->add_dim((shared_cast<IntConst>($1))->get_value()); $$ = $2; }

// ArrayDim 是 IntConst 指针
ArrayDim : "[" IntConst "]" { $$ = $2; }

// 根据定义 不会出现 {{{{{ }}}}} 必须有 {{{Exp}}} 或者是 {} 单个
InitVal : Exp { $$ = new InitVal(shared_cast<Node>($1)); }
    | "{" "}" { $$ = new InitVal(); }
    | "{" InitValList "}" { $$ = $2; }
    ;

// /* empty */ { $$ = new InitVal(); }
InitValList : InitVal { $$ = new InitVal(shared_cast<InitVal>($1)); }
    | InitVal "," InitValList { static_cast<InitVal*>($3)->add_sub(shared_cast<InitVal>($1)); $$ = $3; }
    ;
//这里有个bug 就是可以生成 {1, 2, 3,}

// 同样的，由于 union 中的类型不能是 std::shared_ptr
// 而 FuncDef 初始化时需要传入一个 BlockPtr (std::shared_ptr<Block>)
// 所以我们需要通过 static_cast 来转换类型
// 再用 std::shared_ptr 包装一下
// 才能传入 FuncDef 的构造函数
// 这里 shared_cast 是一个自定义的模板函数，用于将普通指针转换成 std::shared_ptr
// 相当于 std::shared_ptr<T>(static_cast<T*>(ptr))
FuncDef : "int" IDENT "(" ")" Block { $$ = new FuncDef(BasicType::Int, $2, shared_cast<Block>($5)); }
    | "void" IDENT "(" ")" Block { $$ = new FuncDef(BasicType::Void, $2, shared_cast<Block>($5)); }
    | "int" IDENT "(" FuncFParams ")" Block { $$ = new FuncDef(BasicType::Int, $2, shared_cast<Block>($6), static_cast<FuncDef*>($4)->get_params()); }
    | "void" IDENT "(" FuncFParams ")" Block { $$ = new FuncDef(BasicType::Void, $2, shared_cast<Block>($6), static_cast<FuncDef*>($4)->get_params()); }
    ;

FuncFParams : FuncFParam { $$ = new FuncDef(shared_cast<Param>($1)); }
    | FuncFParams "," FuncFParam { static_cast<FuncDef*>($1)->add_param(shared_cast<Param>($3)); $$ = $1; }
    ;

FuncFParam : "int" IDENT { $$ = new Param($2); }
    | "int" IDENT "[" "]" { $$ = new Param($2, -1); }
    | "int" IDENT "[" "]" ArrayDims { $$ = new Param($2, -1); static_cast<Param*>($$)->add_dims((shared_cast<VarDef>($5))->get_dims()); }
    ;

Block : "{" "}" { $$ = new Block(); }
    | "{" BlockItems "}" { $$ = $2; }
    ;

BlockItems : BlockItem { $$ = new Block(NodePtr($1)); }
    | BlockItems BlockItem { static_cast<Block*>($1)->add_stmt(NodePtr($2)); $$ = $1; }
    ;

BlockItem : Stmt { $$ = $1; }
    | Decl { $$ = $1; }
    ;

Stmt : LVal "=" Exp ";" { $$ = new AssignStmt(shared_cast<LVal>($1), NodePtr($3)); }
    | ";" { $$ = new ExpStmt(); }
    | Exp ";" { $$ = $1; }
    | "return" Exp ";" { $$ = new ReturnStmt(NodePtr($2)); }
    | "return" ";" { $$ = new ReturnStmt(); }
    | "if" "(" Cond ")" Stmt %prec LOWER_THAN_ELSE { $$ = new IfStmt(NodePtr($3), NodePtr($5)); } 
    | "if" "(" Cond ")" Stmt "else" Stmt { $$ = new IfStmt(NodePtr($3), NodePtr($5), NodePtr($7)); }
    | "while" "(" Cond ")" Stmt { $$ = new WhileStmt(NodePtr($3), NodePtr($5)); }
    | Block { $$ = $1; }
    ;


Cond : LOrExp { $$ = $1; }
    ;

LOrExp : LAndExp { $$ = $1; }
    | LOrExp "||" LAndExp { $$ = new BinaryExp(BinaryOp::LOr, NodePtr($1), NodePtr($3)); }
    ;

LAndExp : EqExp { $$ = $1; }
    | LAndExp "&&" EqExp { $$ = new BinaryExp(BinaryOp::LAnd, NodePtr($1), NodePtr($3)); }
    ;

EqExp : RelExp { $$ = $1; }
    | EqExp "==" RelExp { $$ = new BinaryExp(BinaryOp::EQ, NodePtr($1), NodePtr($3)); }
    | EqExp "!=" RelExp { $$ = new BinaryExp(BinaryOp::NEQ, NodePtr($1), NodePtr($3)); }
    ;

RelExp : AddExp { $$ = $1; }
    | RelExp "<" AddExp { $$ = new BinaryExp(BinaryOp::LT, NodePtr($1), NodePtr($3)); }
    | RelExp ">" AddExp { $$ = new BinaryExp(BinaryOp::GT, NodePtr($1), NodePtr($3)); }
    | RelExp "<=" AddExp { $$ = new BinaryExp(BinaryOp::LE, NodePtr($1), NodePtr($3)); }
    | RelExp ">=" AddExp { $$ = new BinaryExp(BinaryOp::GE, NodePtr($1), NodePtr($3)); }
    ;

Exp : AddExp { $$ = $1; }
    ;

LVal : IDENT { $$ = new LVal($1); }
    | IDENT LValArrayDims { $$ = new LVal($1); }
    ;

LValArrayDims : LValArrayDim { $$ = $1; }
    | LValArrayDim LValArrayDims { $$ = $1; }

LValArrayDim : "[" Exp "]" { $$ = $2; }


PrimaryExp : LVal { $$ = $1; }
    | IntConst { $$ = $1; }
    | "(" Exp ")" { $$ = $2; }
    ;

IntConst : INTCONST { $$ = new IntConst($1); }
    ;

UnaryExp : PrimaryExp { $$ = $1; }
    | IDENT "(" ")" { $$ = new FuncCall($1); }
    | IDENT "(" FuncRParams ")" { static_cast<FuncCall*>($3)->name = $1; $$ = $3; }
    | UnaryOp UnaryExp { $$ = new UnaryExp($1, NodePtr($2)); }
    ;

UnaryOp : "+" { $$ = BinaryOp::Add; }
    | "-" { $$ = BinaryOp::Sub; }
    | "!" { $$ = BinaryOp::Not; }
    ;

FuncRParams : Exp { $$ = new FuncCall(NodePtr($1)); }
    | FuncRParams "," Exp { static_cast<FuncCall*>($1)->add_arg(NodePtr($3)); $$ = $1; }
    ;

MulExp : UnaryExp { $$ = $1; }
    | MulExp "*" UnaryExp { $$ = new BinaryExp(BinaryOp::Mul, NodePtr($1), NodePtr($3)); }
    | MulExp "/" UnaryExp { $$ = new BinaryExp(BinaryOp::Div, NodePtr($1), NodePtr($3)); }
    | MulExp "%" UnaryExp { $$ = new BinaryExp(BinaryOp::Mod, NodePtr($1), NodePtr($3)); }
    ;

AddExp : MulExp { $$ = $1; }
    | AddExp "+" MulExp { $$ = new BinaryExp(BinaryOp::Add, NodePtr($1), NodePtr($3)); }
    | AddExp "-" MulExp { $$ = new BinaryExp(BinaryOp::Sub, NodePtr($1), NodePtr($3)); }
    ;

%%

void yyerror(const char *s) {
    printf("error: %s\n", s);
}
