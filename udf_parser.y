%{
//-- don't change *any* of these: if you do, you'll break the compiler.
#include <algorithm>
#include <memory>
#include <cstring>
#include <cdk/compiler.h>
#include <cdk/types/types.h>
#include ".auto/all_nodes.h"
#define LINE                         compiler->scanner()->lineno()
#define yylex()                      compiler->scanner()->scan()
#define yyerror(compiler, s)         compiler->scanner()->error(s)
//-- don't change *any* of these --- END!

struct tensor_attr {
  std::vector<std::size_t> dims;
  cdk::sequence_node *values;
  int num_values;
};
%}

%parse-param {std::shared_ptr<cdk::compiler> compiler}

%union {
  //--- don't change *any* of these: if you do, you'll break the compiler.
  YYSTYPE() : type(cdk::primitive_type::create(0, cdk::TYPE_VOID)) {}
  ~YYSTYPE() {}
  YYSTYPE(const YYSTYPE &other) { *this = other; }
  YYSTYPE& operator=(const YYSTYPE &other) { type = other.type; return *this; }

  std::shared_ptr<cdk::basic_type> type;        /* expression type */
  //-- don't change *any* of these --- END!

  int                   i;          /* integer value */
  double                d;          /* real value */
  std::string          *s;          /* symbol name or string literal */
  
  cdk::basic_node      *node;       /* node pointer */
  cdk::sequence_node   *sequence;
  cdk::expression_node *expression; /* expression nodes */
  cdk::lvalue_node     *lvalue;

  udf::variable_declaration_node *vardec; /* variable declaration nodes */
  udf::function_declaration_node *fundec; /* function declaration nodes */
  udf::function_definition_node *fundef; /* function definition nodes */
  udf::block_node *block; /* block nodes */
  udf::tensor_node *tensor; /* tensor nodes */
  
  std::vector<std::size_t> *dims_vector;

  struct tensor_attr *tensor_attr; /* tensor attributes */
};

%token tTYPE_STRING tTYPE_INT tTYPE_REAL tTYPE_POINTER tTYPE_AUTO tTYPE_TENSOR tTYPE_VOID
%token tSIZEOF tOBJECTS
%token tPUBLIC tFORWARD tPRIVATE
%token tIF tELIF tELSE
%token tFOR tBREAK tCONTINUE tRETURN
%token tAND tOR tNE tLE tGE tEQ
%token tT_CAPACITY tT_RANK tT_DIMS tT_DIM tT_RESHAPE tT_CONTRACTION
%token tINPUT tWRITE tWRITELN

%token<i> tINTEGER
%token<d> tREAL
%token<s> tID tSTRING
%token<expression> tNULLPTR

%type<node>     declaration  instruction  iffalse 
%type<sequence> declarations instructions opt_instructions file

%type<sequence> local_vardecs  argdecs fordecs opt_forinit opt_vardecs
%type<vardec>   local_vardec   argdec  fordec vardec

%type<sequence>   expressions opt_expressions 
%type<expression> expression  other_expr opt_initializer 

%type<fundec> fundec 
%type<fundef> fundef
%type<block>  block
%type<lvalue> lvalue

%type<s> string
%type<type> data_type void_type
%type<tensor> tensor 
%type<dims_vector> dims
%type<tensor_attr> tensor_value tensor_values

%nonassoc tIF
%nonassoc tIFX
%nonassoc tELIF tELSE

%right '='
%left tOR
%left tAND
%nonassoc '~'
%left tNE tEQ
%left '<' tLE tGE '>'
%left '+' '-'
%left '*' '/' '%'
%left tT_CONTRACTION
%left '.' '@' '['
%nonassoc tUNARY

%%

file          : declarations                      { compiler->ast($$ = $1); }
              ;

declarations  : declaration                       { $$ = new cdk::sequence_node(LINE, $1); }
              | declarations declaration          { $$ = new cdk::sequence_node(LINE, $2, $1); }
              ;

declaration   : vardec ';'                        { $$ = $1; }
              | fundec                            { $$ = $1; }
              | fundef                            { $$ = $1; }
              ;

vardec        : local_vardec                      { $$ = $1; }
              | tFORWARD data_type  tID
              {
                $$ = new udf::variable_declaration_node(LINE, tFORWARD, $2, *$3, nullptr);
                delete $3;
              }
              | tPUBLIC  data_type  tID opt_initializer
              {
                $$ = new udf::variable_declaration_node(LINE, tPUBLIC, $2, *$3, $4);
                delete $3;
              }
              | tPUBLIC  tTYPE_AUTO tID '=' expression 
              {
                $$ = new udf::variable_declaration_node(LINE, tPUBLIC, *$3, $5);
                delete $3;
              }
              ;

local_vardec  : data_type  tID opt_initializer
              {
                $$ = new udf::variable_declaration_node(LINE, tPRIVATE, $1, *$2, $3);
                delete $2;
              }
              | tTYPE_AUTO tID '=' expression 
              {
                $$ = new udf::variable_declaration_node(LINE, tPRIVATE, *$2, $4);
                delete $2;
              }

data_type     : tTYPE_STRING                      { $$ = cdk::primitive_type::create(4, cdk::TYPE_STRING); }
              | tTYPE_INT                         { $$ = cdk::primitive_type::create(4, cdk::TYPE_INT); }
              | tTYPE_REAL                        { $$ = cdk::primitive_type::create(8, cdk::TYPE_DOUBLE); }
              | tTYPE_POINTER '<' data_type '>'   { $$ = cdk::reference_type::create(4, $3); }
              | tTYPE_POINTER '<' tTYPE_AUTO '>'  { $$ = cdk::reference_type::create(4, nullptr); }
              | tTYPE_TENSOR '<' dims '>'         { $$ = cdk::tensor_type::create(*$3); delete $3; } 
              ;

dims          : /* empty */                       { $$ = new std::vector<std::size_t>(); }
              | tINTEGER                          { $$ = new std::vector<std::size_t>(); $$->push_back($1 * 8); }
              | dims ',' tINTEGER                 { $$ = $1; $$->push_back($3 * 8); }
              ;

opt_initializer : /* empty */                     { $$ = nullptr;}
                | '=' expression                  { $$ = $2;}
                ;

fundec        :          data_type  tID '(' argdecs ')'
              {
                $$ = new udf::function_declaration_node(LINE, tPRIVATE, $1, *$2, $4);
                delete $2;
              }
              | tFORWARD data_type  tID '(' argdecs ')'
              {
                $$ = new udf::function_declaration_node(LINE, tFORWARD,  $2, *$3, $5);
                delete $3;
              }
              | tPUBLIC  data_type  tID '(' argdecs ')'
              {
                $$ = new udf::function_declaration_node(LINE, tPUBLIC,  $2, *$3, $5);
                delete $3;
              }
              |          tTYPE_AUTO tID '(' argdecs ')'
              {
                $$ = new udf::function_declaration_node(LINE, tPRIVATE, *$2, $4);
                delete $2;
              }
              | tFORWARD tTYPE_AUTO tID '(' argdecs ')'
              {
                $$ = new udf::function_declaration_node(LINE, tFORWARD, *$3, $5);
                delete $3;
              }
              | tPUBLIC  tTYPE_AUTO tID '(' argdecs ')'
              { 
                $$ = new udf::function_declaration_node(LINE, tPUBLIC, *$3, $5);
                delete $3;
              }
              |          void_type  tID '(' argdecs ')'
              {
                $$ = new udf::function_declaration_node(LINE, tPRIVATE, $1, *$2, $4);
                delete $2;
              }
              | tFORWARD void_type  tID '(' argdecs ')'
              {
                $$ = new udf::function_declaration_node(LINE, tFORWARD, $2, *$3, $5);
                delete $3;
              }
              | tPUBLIC  void_type  tID '(' argdecs ')'
              { 
                $$ = new udf::function_declaration_node(LINE, tPUBLIC, $2, *$3, $5);
                delete $3;
              }
              ;

fundef        :         data_type  tID '(' argdecs ')' block
              {
                $$ = new udf::function_definition_node(LINE, tPRIVATE, $1, *$2, $4, $6);
                delete $2;
              }
              | tPUBLIC data_type  tID '(' argdecs ')' block
              {
                $$ = new udf::function_definition_node(LINE, tPUBLIC,  $2, *$3, $5, $7);
                delete $3;
              }
              |         tTYPE_AUTO tID '(' argdecs ')' block
              {
                $$ = new udf::function_definition_node(LINE, tPRIVATE, *$2, $4, $6);
                delete $2;
              }
              | tPUBLIC tTYPE_AUTO tID '(' argdecs ')' block
              {
                $$ = new udf::function_definition_node(LINE, tPUBLIC, *$3, $5, $7);
                delete $3;
              }
              |         void_type  tID '(' argdecs ')' block
              {
                $$ = new udf::function_definition_node(LINE, tPRIVATE, $1, *$2, $4, $6);
                delete $2;
              }
              | tPUBLIC void_type  tID '(' argdecs ')' block
              {
                $$ = new udf::function_definition_node(LINE, tPUBLIC, $2, *$3, $5, $7);
                delete $3;
              }
              ;

argdecs       : /* empty */                       { $$ = new cdk::sequence_node(LINE);  }
              | argdec                            { $$ = new cdk::sequence_node(LINE, $1);     }
              | argdecs ',' argdec                { $$ = new cdk::sequence_node(LINE, $3, $1); }
              ;

argdec        : data_type tID
              {
                $$ = new udf::variable_declaration_node(LINE, tPRIVATE, $1, *$2);
                delete $2;
              }
              ;

block         : '{' opt_vardecs opt_instructions '}'        { $$ = new udf::block_node(LINE, $2, $3); }
              ;

opt_vardecs   : /* empty */                       { $$ = NULL; }
              | local_vardecs                     { $$ = $1; }
              ;

local_vardecs : local_vardec ';'                  { $$ = new cdk::sequence_node(LINE, $1); }
              | local_vardecs local_vardec ';'    { $$ = new cdk::sequence_node(LINE, $2, $1); }
              ;

opt_instructions  : /* empty */                   { $$ = new cdk::sequence_node(LINE);  }
                  | instructions                  { $$ = new cdk::sequence_node(LINE, $1); }
                  ;

instructions  : instruction                       { $$ = new cdk::sequence_node(LINE, $1);     }
              | instructions instruction          { $$ = new cdk::sequence_node(LINE, $2, $1); }
              ;

instruction   : expression ';'                                  { $$ = new udf::evaluation_node(LINE, $1); }
              | tWRITE   expressions ';'                        { $$ = new udf::write_node(LINE, $2, false); }
              | tWRITELN expressions ';'                        { $$ = new udf::write_node(LINE, $2, true); }
              | tBREAK                                          { $$ = new udf::break_node(LINE);  }
              | tCONTINUE                                       { $$ = new udf::continue_node(LINE); }
              | tRETURN ';'                                     { $$ = new udf::return_node(LINE); }
              | tRETURN expression ';'                          { $$ = new udf::return_node(LINE, $2); }
              | tIF '(' expression ')' instruction %prec tIFX   { $$ = new udf::if_node(LINE, $3, $5); }
              | tIF '(' expression ')' instruction iffalse      { $$ = new udf::if_else_node(LINE, $3, $5, $6); }
              | tFOR '(' opt_forinit ';' opt_expressions ';' opt_expressions ')' instruction { $$ = new udf::for_node(LINE, $3, $5, $7, $9); }
              | block                                           { $$ = $1; }
              ;

iffalse       : tELSE instruction                               { $$ = $2; }
              | tELIF '(' expression ')' instruction %prec tIFX { $$ = new udf::if_node(LINE, $3, $5); }
              | tELIF '(' expression ')' instruction iffalse    { $$ = new udf::if_else_node(LINE, $3, $5, $6); }
              ;

opt_forinit   : /* empty */                       { $$ = new cdk::sequence_node(LINE); }
              | fordecs                           { $$ = $1; }
              | tTYPE_AUTO tID '=' expression
              {
                $$ = new cdk::sequence_node(LINE, new udf::variable_declaration_node(LINE, tPRIVATE, *$2, $4));
                delete $2;
              }
              | expressions                       { $$ = $1; }
              ;

fordecs       : fordec                            { $$ = new cdk::sequence_node(LINE, $1); }
              | fordecs ',' fordec                { $$ = new cdk::sequence_node(LINE, $3, $1); }
              ;

fordec        : data_type tID '=' expression      { $$ = new udf::variable_declaration_node(LINE, tPRIVATE,  $1, *$2, $4); }
              ;

lvalue        : tID                               { $$ = new cdk::variable_node(LINE, *$1); }
              | expression '@' '(' expressions ')'{ $$ = new udf::index_tensor_node(LINE, $1, $4); }
              | expression '[' expression ']'     { $$ = new udf::index_ptr_node(LINE, $1, $3); }
              ;

expression    : other_expr                        { $$ = $1; }
              | tensor                            { $$ = $1; }
              ;

other_expr    : tINTEGER                          { $$ = new cdk::integer_node(LINE, $1); }
              | tREAL                             { $$ = new cdk::double_node(LINE, $1); }
              | string                            { $$ = new cdk::string_node(LINE, $1); }
              | tNULLPTR                          { $$ = new udf::nullptr_node(LINE); }
              /* LEFT VALUES */
              | lvalue                            { $$ = new cdk::rvalue_node(LINE, $1); }
              /* ASSIGNMENTS */
              | lvalue '=' expression             { $$ = new cdk::assignment_node(LINE, $1, $3); }
              /* ARITHMETIC EXPRESSIONS */
              | expression '+' expression         { $$ = new cdk::add_node(LINE, $1, $3); }
              | expression '-' expression         { $$ = new cdk::sub_node(LINE, $1, $3); }
              | expression '*' expression         { $$ = new cdk::mul_node(LINE, $1, $3); }
              | expression '/' expression         { $$ = new cdk::div_node(LINE, $1, $3); }
              | expression '%' expression         { $$ = new cdk::mod_node(LINE, $1, $3); }
              /* LOGICAL EXPRESSIONS */
              | expression  '<' expression        { $$ = new cdk::lt_node(LINE, $1, $3); }
              | expression tLE  expression        { $$ = new cdk::le_node(LINE, $1, $3); }
              | expression tEQ  expression        { $$ = new cdk::eq_node(LINE, $1, $3); }
              | expression tGE  expression        { $$ = new cdk::ge_node(LINE, $1, $3); }
              | expression  '>' expression        { $$ = new cdk::gt_node(LINE, $1, $3); }
              | expression tNE  expression        { $$ = new cdk::ne_node(LINE, $1, $3); }
              /* LOGICAL EXPRESSIONS */
              | expression tAND  expression       { $$ = new cdk::and_node(LINE, $1, $3); }
              | expression tOR   expression       { $$ = new cdk::or_node (LINE, $1, $3); }
              /* UNARY EXPRESSION */
              | '-' expression %prec tUNARY       { $$ = new cdk::unary_minus_node(LINE, $2); }
              | '+' expression %prec tUNARY       { $$ = $2; }
              | '~' expression                    { $$ = new cdk::not_node(LINE, $2); }
              /* OTHER EXPRESSION */
              | tINPUT                            { $$ = new udf::input_node(LINE); }
              /* OTHER EXPRESSION */
              | tID '(' opt_expressions ')'       { $$ = new udf::function_call_node(LINE, *$1, $3); delete $1;}
              | tSIZEOF '(' expression ')'        { $$ = new udf::sizeof_node(LINE, $3); }
              /* TENSOR EXPRESSION */
              | expression '.' tT_CAPACITY                        { $$ = new udf::tensor_capacity_node(LINE, $1); }
              | expression '.' tT_DIMS                            { $$ = new udf::tensor_dims_node(LINE, $1); }
              | expression '.' tT_DIM         '(' expression ')'  { $$ = new udf::tensor_dim_node(LINE, $1, $5);  }
              | expression '.' tT_RANK                            { $$ = new udf::tensor_rank_node(LINE, $1);}
              | expression '.' tT_RESHAPE     '(' dims ')'        { $$ = new udf::tensor_reshape_node(LINE, $1, *$5); delete $5; }
              | expression tT_CONTRACTION expression              { $$ = new udf::tensor_contraction_node(LINE, $1, $3); }
              /* OTHER EXPRESSION */
              | '(' expression ')'                { $$ = $2; }
              | tOBJECTS '(' expression ')'       { $$ = new udf::objects_node(LINE, $3); }
              | lvalue '?'                        { $$ = new udf::address_of_node(LINE, $1); }
              ;

expressions   : expression                        { $$ = new cdk::sequence_node(LINE, $1); }
              | expressions ',' expression        { $$ = new cdk::sequence_node(LINE, $3, $1); }
              ;

opt_expressions : /* empty */                     { $$ = new cdk::sequence_node(LINE); }
                | expressions                     { $$ = $1; }
                ;

void_type     : tTYPE_VOID                        { $$ = cdk::primitive_type::create(0, cdk::TYPE_VOID); }
              ;

string        : tSTRING                           { $$ = $1; }
              | string tSTRING                    { $$ = $1; $$->append(*$2); delete $2; }
              ;

tensor        : '[' tensor_values ']'
              { 
                $2->dims.insert($2->dims.begin(), $2->num_values * 8);
                $$ = new udf::tensor_node(LINE, $2->dims, $2->values);
                delete $2;
              }
              ;

tensor_values : /* empty */
              {
                $$ = new tensor_attr();
                $$->values = new cdk::sequence_node(LINE);
                $$->dims = {};
                $$->num_values = 0;
              }
              | tensor_value                      { $$ = $1; }
              | tensor_values ',' tensor_value
              {
                if($1->dims != $3->dims) {
                  std::cerr << "Syntax error: Incompatible tensor dimensions.\n" << std::endl;
                  exit(1);
                }
                $$ = new tensor_attr();
                $$->values = new cdk::sequence_node(LINE, $3->values, $1->values);
                $$->num_values = $1->num_values + 1;
                $$->dims = $1->dims;

                delete $1;
                delete $3;
              }
              ;

tensor_value  : tensor
              {
                $$ = new tensor_attr();

                auto tensor_type = std::dynamic_pointer_cast<cdk::tensor_type>($1->type());
                if (!tensor_type) {
                  std::cerr << "Internal error: expected tensor type but received incompatible type." << std::endl;
                  exit(1);
                }
                $$->dims = tensor_type->dims();

                $$->values = $1->values();
                $$->num_values = 1;
              }
              | other_expr
              {
                $$ = new tensor_attr();
                $$->values = new cdk::sequence_node(LINE, $1);
                $$->dims = {};
                $$->num_values = 1;
              }
              ;

%%

