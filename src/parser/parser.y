%{
#include <cstdio>
#include <memory>
#include <string>
#include <iostream>
#include "ast/tree.hpp"
#define YYDEBUG 1
void yyerror(const char *s);
extern int yylex(void);
using namespace AST;
extern NodePtr root;
%}

// yylval 的定义, 我们把它定义成了一个联合体 (union)
// 因为 token 的值有的是字符串指针, 有的是整数
// 之前我们在 lexer 中用到的 str_val 和 int_val 就是在这里被定义的
// 这里的 union 是一种特殊的数据结构，所有成员共享相同的内存地址
// 因此只能存储一个成员的值，且占用的内存大小等于其最大成员的大小
// union 中的类型必须是 trivially copyable 的类型
// 也就是说不能包含有自定义的构造函数、析构函数、虚函数等
// 符合这个条件的类型有基本数据类型、指针、C 结构体、枚举等
// 因此我们不能使用 std::shared_ptr，而只能使用普通指针
%union{
    int int_val;
    char *str_val;
    BinaryOp op;
    AST::Node *node;
}

%start AstRoot
%token ADD "+"
%token SUB "-"
%token ASSIGN "="
%token SEMICOLON ";"
%token COMMA ","
%token LPAREN "("
%token RPAREN ")"
%token LBRACE "{"
%token RBRACE "}"
%token INT "int"
%token RETURN "return"
%token <str_val> IDENT
%token <int_val> INTCONST

%type <node> AstRoot CompUnit Decl VarDecl VarDefs VarDef FuncDef Block BlockItem BlockItems Stmt Exp LVal PrimaryExp IntConst UnaryExp FuncRParams MulExp AddExp
%type <op> UnaryOp

%%

// 由于我们在后续代码中所有指针都是 std::shared_ptr
// 而在 union 中没法直接定义 std::shared_ptr
// 所以我们在这里需要将普通的指针转换成 std::shared_ptr
AstRoot : CompUnit { root = NodePtr($1); }
    ;

// 同样的，由于 union 中的类型不能是 std::shared_ptr
// 而 CompUnit 初始化时需要传入一个 FuncDefPtr (std::shared_ptr<FuncDef>)
// 所以我们需要通过 static_cast 来转换类型
// 在用 std::shared_ptr 包装一下
// 才能传入 CompUnit 的构造函数
CompUnit : FuncDef { $$ = new CompUnit(FuncDefPtr(static_cast<FuncDef*>($1))); }
    | CompUnit FuncDef { static_cast<CompUnit*>($1)->add_unit(FuncDefPtr(static_cast<FuncDef*>($2))); }
    ;

Decl : VarDecl { $$ = $1; }
    ;

// 同样的，由于 union 中的类型不能是 std::shared_ptr
// 只是普通的指针，所以我们需要通过 static_cast 来转换类型
// 才能访问到对应的成员和函数
VarDecl : "int" VarDefs ";" { static_cast<VarDecl *>($2)->btype = BasicType::Int; $$ = $2; }
    ;

VarDefs : VarDef { $$ = new VarDecl(VarDefPtr(static_cast<VarDef*>($1))); }
    | VarDefs "," VarDef { static_cast<VarDecl*>($1)->add_def(VarDefPtr(static_cast<VarDef*>($3))); }
    ;

VarDef : IDENT { $$ = new VarDef($1); }
    ;

FuncDef : "int" IDENT "(" ")" Block { $$ = new FuncDef(BasicType::Int, $2, BlockPtr(static_cast<Block*>($5))); }
    ;

Block : "{" "}" { $$ = new Block(); }
    | "{" BlockItems "}" { $$ = $2; }
    ;

BlockItems : BlockItem { $$ = new Block(NodePtr($1)); }
    | BlockItems BlockItem { static_cast<Block*>($1)->add_stmt(NodePtr($2)); }
    ;

BlockItem : Stmt { $$ = $1; }
    | Decl { $$ = $1; }
    ;

Stmt : LVal "=" Exp ";" { $$ = new AssignStmt(LValPtr(static_cast<LVal*>($1)), NodePtr($3)); }
    | Exp ";" { $$ = $1; }
    | "return" Exp ";" { $$ = new ReturnStmt(NodePtr($2)); }
    ;

Exp : AddExp { $$ = $1; }
    ;

LVal : IDENT { $$ = new LVal($1); }
    ;

PrimaryExp : LVal { $$ = $1; }
    | IntConst { $$ = $1; }
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
    ;

FuncRParams : Exp { $$ = new FuncCall(NodePtr($1)); }
    | FuncRParams "," Exp { static_cast<FuncCall*>($1)->add_arg(NodePtr($3)); }
    ;

MulExp : UnaryExp { $$ = $1; }
    ;

AddExp : MulExp { $$ = $1; }
    | AddExp "+" MulExp { $$ = new BinaryExp(BinaryOp::Add, NodePtr($1), NodePtr($3)); }
    ;

%%

void yyerror(const char *s) {
    printf("error: %s\n", s);
}
