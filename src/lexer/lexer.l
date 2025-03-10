%option noinput
%option nounput
%option noyywrap
%option yylineno

%{
// yylex()作用 读取下一个词法单元 返回它的类型
// 这部分内容会被复制到lexer.yy.c最前面
#include <cstring>
#include <cstdlib>
#include <stdexcept>
#include <string>
#include <iostream>

// 包含 Bison 生成的解析器头文件，里面有 token 枚举及 yylval 的声明 
// yylval 被定义为全局变量，用于在词法分析器（Flex）和语法分析器（Bison）之间传递 token 的语义值
/*
yylval 这是由 Bison 定义的全局变量 用来传递词法分析器（Flex）识别的 token 的语义信息给语法分析器。
当 Flex 匹配到一个 token 时，会将与该 token 相关的数据（比如整数值、标识符字符串等）存入 yylval 中，然后返回 token 类型
比如说 当识别整数常量时，代码中会写 yylval.int_val = std::strtol(yytext, nullptr, 10);
这就是把解析后的数值存入 yylval，然后返回 INTCONST 标识符供语法分析器使用。
*/
// 注意根据你的 parser.y 实际输出名称来修改，例如 "parser.tab.hh"、"parser.tab.hpp" 等
#include "ast/tree.hpp"
#include "parser/parser.tab.hh"

extern FILE *yyin;  // 在 main.cpp 或其它地方定义
/*
这是一个 FILE 指针，用于指定词法分析器的输入流。

默认情况下，yyin 指向标准输入（stdin），如果不进行修改，Flex 会从标准输入中读取数据。
如果我们将main函数独立设为一个文件，则需要声明yyin为外部变量：extern FILE* yyin。
如果你希望从文件中读取数据，可以在main中将 yyin 指向一个文件，例如：
yyin = fopen("input.txt", "r");
这使得 Flex 能够从指定文件中进行词法分析，而不是从标准输入读取数据。
*/
%}

/** ====================== 别名声明 ====================== **/
digit           [0-9]
letter          [a-zA-Z_]
identifier      {letter}({letter}|{digit})*
blank           [ \t\r\n]
linecomment     \/\/[^\n]*
comment         \/\*([^*]|\*[^/])*\*\/
/** 如果遇到未终止的comment 应该会去匹配 / * 这些字符 **/

/*
 * SysY 中要求支持：0归类在8进制里面 10进制里面总是以非0开头
 * 10进制：形如 [1-9][0-9]*
 * 8进制：形如 0[0-7]*
 * 16进制：形如 0[xX][0-9a-fA-F]+
 */
hex_const       0[xX][0-9a-fA-F]+
oct_const       0[0-7]*
dec_const       [1-9][0-9]*

%%

"int"           { return INT; }
"void"          { return VOID; }
"if"            { return IF; }
"else"          { return ELSE; }
"while"         { return WHILE; }
"return"        { return RETURN; }


{hex_const} {
    // strtol会解析 0x 或 0X 前缀并以16进制转换
    yylval.int_val = std::strtol(yytext, nullptr, 16);
    return INTCONST;
}

{oct_const} {
    // strtol 遇到前缀 0 会以八进制解析
    yylval.int_val = std::strtol(yytext, nullptr, 8);
    return INTCONST;
}

{dec_const} {
    yylval.int_val = std::strtol(yytext, nullptr, 10);
    return INTCONST;
}

{identifier} {
    yylval.str_val = strdup(yytext);
    return IDENT;
}

"=="            { return EQ; }
"!="            { return NEQ; }
"<="            { return LE; }
">="            { return GE; }
"<"             { return LT; }
">"             { return GT; }
"&&"            { return AND; }
"||"            { return OR; }
"!"             { return NOT; }

"+"             { return ADD; }
"-"             { return SUB; }
"*"             { return MUL; }
"/"             { return DIV; }
"%"             { return MOD; }
"="             { return ASSIGN; }

"("             { return LPAREN; }
")"             { return RPAREN; }
"{"             { return LBRACE; }
"}"             { return RBRACE; }
"["             { return LBRACK; }
"]"             { return RBRACK; }
";"             { return SEMICOLON; }
","             { return COMMA; }

{linecomment}   { }
{comment}       { }
{blank}         { }

. {
    throw std::runtime_error(
        "Unknown token '" + std::string(yytext) +
        "' at line " + std::to_string(yylineno)
    );
}

%%