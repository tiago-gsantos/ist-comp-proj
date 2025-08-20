%option c++ prefix="udf_scanner_" outfile="udf_scanner.cpp"
%option stack noyywrap yylineno 8bit debug
%{ 
// make relevant includes before including the parser's tab file
#include <string>
#include <sstream>
#include <cdk/ast/sequence_node.h>
#include <cdk/ast/expression_node.h>
#include <cdk/ast/lvalue_node.h>
#include "udf_parser.tab.h"

// output stream for building string literals
static std::ostringstream strlit;

// don't change this
#define yyerror LexerError
%}

%x X_COMMENT
%x X_STRING X_HEX_INT X_BACKSLASH
%%
  yydebug=1; set_debug(1);


  /* =========== */
  /* Comentarios */
  /* =========== */
"//".*$                         ;/* match the whole line (it is ignored) */

"/*"                            yy_push_state(X_COMMENT);
<X_COMMENT>"/*"                 yy_push_state(X_COMMENT);
<X_COMMENT>"*/"                 yy_pop_state();
<X_COMMENT>.|"\n"               ;  /* ignore inside comments */


  /* ============== */
  /* Tipos de dados */
  /* ============== */
"int"                           return tTYPE_INT;
"real"                          return tTYPE_REAL;
"string"                        return tTYPE_STRING;
"tensor"                        return tTYPE_TENSOR;
"auto"                          return tTYPE_AUTO;
"void"                          return tTYPE_VOID;
"ptr"                           return tTYPE_POINTER;
[<>]                            return *yytext;


  /* ================================================= */
  /*                     Nullptr                       */
  /* Expressões resultantes de avaliação de operadores */
  /* ================================================= */
"nullptr"                       return tNULLPTR;
"objects"                       return tOBJECTS;
"sizeof"                        return tSIZEOF;


  /* ================ */
  /* Símbolos Globais */
  /* ================ */
"public"                        return tPUBLIC;
"forward"                       return tFORWARD;


  /* ===================== */
  /* Instrução condicional */
  /* ===================== */
"if"                            return tIF; 
"elif"                          return tELIF;
"else"                          return tELSE;


  /* ======================== */
  /* Instrução de iteração    */
  /* Instrução de terminação  */
  /* Instrução de continuação */
  /* Instrução de retorno     */
  /* ======================== */
"for"                           return tFOR; 
"break"                         return tBREAK;
"continue"                      return tCONTINUE;
"return"                        return tRETURN;


  /* ======================== */
  /* Operadores de expressões */
  /* ======================== */
[()\[\]]                        return *yytext;
[-+*/%~<>?=]                    return *yytext;
">="                            return tGE; 
"<="                            return tLE; 
"=="                            return tEQ; 
"!="                            return tNE;
"&&"                            return tAND;
"||"                            return tOR; 


  /* ================================== */
  /* Expressões dependentes de tensores */
  /* ================================== */
"capacity"                      return tT_CAPACITY;
"rank"                          return tT_RANK;
"dims"                          return tT_DIMS;
"dim"                           return tT_DIM;
"reshape"                       return tT_RESHAPE;
"@"                             return *yytext;
"**"                            return tT_CONTRACTION;
"\."                            return *yytext;


  /* ============================ */
  /* Delimitadores e terminadores */
  /* ============================ */
[,;(){}]                        return *yytext;


  /* ======================= */
  /* Instruções de impressão */
  /*  Expressão de leitura   */
  /* ======================= */
"write"                         return tWRITE;
"writeln"                       return tWRITELN;
"input"                         return tINPUT;


  /* ======== */
  /* Inteiros */
  /* ======== */
"0x"                            yy_push_state(X_HEX_INT);

<X_HEX_INT>[[:xdigit:]]+        {
                                  try {
                                    yylval.i = std::stoi(yytext, NULL, 16);
                                    yy_pop_state();
                                    return tINTEGER;
                                  } catch (const std::out_of_range &e) {
                                    yyerror("integer overflow");
                                  }
                                }
<X_HEX_INT>.|\n                 yyerror("invalid hexadecimal");

[0-9]+                          {
                                  try {
                                    yylval.i = std::stoi(yytext, NULL, 10);
                                    return tINTEGER;
                                  } catch (const std::out_of_range &e) {
                                    yyerror("integer overflow");
                                  }
                                }


  /* ===== */
  /* Reais */
  /* ===== */
([0-9]*\.[0-9]+|[0-9]+\.[0-9]*)([eE][-+]?[0-9]+)? { 
                                                    try{
                                                      yylval.d = std::stod(yytext);
                                                      return tREAL; 
                                                    } catch (const std::out_of_range &e) {
                                                      yyerror("real overflow");
                                                    }
                                                  }
[0-9]+[eE][-+]?[0-9]+                             {
                                                    try{
                                                      yylval.d = std::stod(yytext);
                                                      return tREAL; 
                                                    } catch (const std::out_of_range &e) {
                                                      yyerror("real overflow");
                                                    }
                                                  }


  /* =============== */
  /* Identificadores */
  /* =============== */
[A-Za-z][A-Za-z0-9_]*           { yylval.s = new std::string(yytext); return tID; }


  /* ===================== */
  /* Cadeias de caracteres */
  /* ===================== */
"\""                            yy_push_state(X_STRING);
<X_STRING>\\                    yy_push_state(X_BACKSLASH);
<X_STRING>"\""                  {
                                  yylval.s = new std::string(strlit.str());
                                  strlit.str("");
                                  yy_pop_state();
                                  return tSTRING;
                                }
<X_STRING>.                     strlit << *yytext;
<X_STRING>\n                    yyerror("newline in string");

<X_BACKSLASH>n                  strlit << '\n'; yy_pop_state();
<X_BACKSLASH>r                  strlit << '\r'; yy_pop_state();
<X_BACKSLASH>t                  strlit << '\t'; yy_pop_state();
<X_BACKSLASH>\\                 strlit << '\\'; yy_pop_state();

<X_BACKSLASH>[[:xdigit:]]{1,2}  {
                                  strlit << (char)(unsigned char)strtoul(yytext, NULL, 16);
                                  yy_pop_state();
                                }
<X_BACKSLASH>.                  strlit << *yytext; yy_pop_state();


  /* ================= */
  /* Outros caracteres */
  /* ================= */
([ \t\r]|\n)                    ; /* ignore remaining white space inside the program */
.                               yyerror(yytext); /* all other chars: error! */

%%
