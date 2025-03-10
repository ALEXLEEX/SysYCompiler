%option noinput
%option nounput
%option noyywrap
%option yylineno

%{
#include <cstdio>
#include <cstdlib>
#include <stdexcept>
#include <string>
#include <iostream>

// =============================
// 1) 先模拟一些 token 枚举，保证能独立编译。
//    这里你可以自己指定一些整数值或者 enum。
//    如果想查看实际词法扫描到什么token，可以打印它们。
#define INT      101
#define VOID     102
#define IF       103
#define ELSE     104
#define WHILE    105
#define RETURN   106
#define INTCONST 107
#define IDENT    108
#define EQ       109
#define NEQ      110
#define LE       111
#define GE       112
#define LT       113
#define GT       114
#define AND      115
#define OR       116
#define NOT      117
#define ADD      118
#define SUB      119
#define MUL      120
#define DIV      121
#define MOD      122
#define ASSIGN   123
#define LPAREN   124
#define RPAREN   125
#define LBRACE   126
#define RBRACE   127
#define LBRACK   128
#define RBRACK   129
#define SEMICOLON 130
#define COMMA    131

// =============================
// 2) 定义全局yylval结构（如果想存int_val/str_val）
//    或者你也可以不定义，直接在动作里打印yytext。
int yylval_int;
char* yylval_str;

// =============================
// 3) 声明 yylex() 原型，以便 main() 调用
int yylex(void);
%}

digit           [0-9]
letter          [a-zA-Z_]
identifier      {letter}({letter}|{digit})*
blank           [ \t\r\n]
linecomment     \/\/[^\n]*
comment         \/\*([^*]|\*[^/])*\*\/

hex_const       0[xX][0-9a-fA-F]+
oct_const       0[0-7]*
dec_const       [1-9][0-9]*

%%

"int"           { std::cout<<"LEX: 'int' -> INT\n"; return INT; }
"void"          { std::cout<<"LEX: 'void' -> VOID\n"; return VOID; }
"if"            { std::cout<<"LEX: 'if' -> IF\n"; return IF; }
"else"          { std::cout<<"LEX: 'else' -> ELSE\n"; return ELSE; }
"while"         { std::cout<<"LEX: 'while' -> WHILE\n"; return WHILE; }
"return"        { std::cout<<"LEX: 'return' -> RETURN\n"; return RETURN; }


{hex_const} {
    long val = std::strtol(yytext, nullptr, 16);
    std::cout<<"LEX: '"<< yytext <<"' -> INTCONST("<<val<<")\n";
    yylval_int = val; 
    return INTCONST;
}


{oct_const} {
    long val = std::strtol(yytext, nullptr, 8);
    std::cout<<"LEX: '"<< yytext <<"' -> INTCONST("<<val<<")\n";
    yylval_int = val;
    return INTCONST;
}


{dec_const} {
    long val = std::strtol(yytext, nullptr, 10);
    std::cout<<"LEX: '"<< yytext <<"' -> INTCONST("<<val<<")\n";
    yylval_int = val;
    return INTCONST;
}


{identifier} {
    std::cout<<"LEX: '"<< yytext <<"' -> IDENT\n";
    yylval_str = strdup(yytext);
    return IDENT;
}


"=="  { std::cout<<"LEX: '==' -> EQ\n";    return EQ; }
"!="  { std::cout<<"LEX: '!=' -> NEQ\n";   return NEQ; }
"<="  { std::cout<<"LEX: '<=' -> LE\n";    return LE; }
">="  { std::cout<<"LEX: '>=' -> GE\n";    return GE; }
"<"   { std::cout<<"LEX: '<' -> LT\n";     return LT; }
">"   { std::cout<<"LEX: '>' -> GT\n";     return GT; }
"&&"  { std::cout<<"LEX: '&&' -> AND\n";   return AND; }
"||"  { std::cout<<"LEX: '||' -> OR\n";    return OR; }
"!"   { std::cout<<"LEX: '!' -> NOT\n";    return NOT; }


"+"   { std::cout<<"LEX: '+' -> ADD\n";    return ADD; }
"-"   { std::cout<<"LEX: '-' -> SUB\n";    return SUB; }
"*"   { std::cout<<"LEX: '*' -> MUL\n";    return MUL; }
"/"   { std::cout<<"LEX: '/' -> DIV\n";    return DIV; }
"%"   { std::cout<<"LEX: '%' -> MOD\n";    return MOD; }
"="   { std::cout<<"LEX: '=' -> ASSIGN\n"; return ASSIGN; }


"(" { std::cout<<"LEX: '(' -> LPAREN\n";   return LPAREN; }
")" { std::cout<<"LEX: ')' -> RPAREN\n";   return RPAREN; }
"{" { std::cout<<"LEX: '{' -> LBRACE\n";   return LBRACE; }
"}" { std::cout<<"LEX: '}' -> RBRACE\n";   return RBRACE; }
"[" { std::cout<<"LEX: '[' -> LBRACK\n";   return LBRACK; }
"]" { std::cout<<"LEX: ']' -> RBRACK\n";   return RBRACK; }
";" { std::cout<<"LEX: ';' -> SEMICOLON\n"; return SEMICOLON; }
"," { std::cout<<"LEX: ',' -> COMMA\n";     return COMMA; }


{linecomment} { /* ignore single-line comment */ }
{comment}     { /* ignore multi-line comment */ }
{blank}       { /* ignore whitespace */ }

. {
    throw std::runtime_error(
        "Unknown token '" + std::string(yytext) +
        "' at line " + std::to_string(yylineno)
    );
}

%%

// 如果用了 noyywrap，就不实现 yywrap()
// int yywrap() { return 1; }

// 4) 提供一个主函数用来测试
int main(int argc, char** argv){
    if(argc > 1) {
        FILE* fp = fopen(argv[1], "r");
        if(!fp){
            std::cerr<<"Cannot open file: "<<argv[1]<<std::endl;
            return 1;
        }
        yyin = fp;
    }

    while(true){
        int t = yylex();
        if(t == 0) {
            std::cout<<"<EOF>\n";
            break;
        }
        // 如果yylex中抛异常，会中断到catch
    }

    return 0;
}
