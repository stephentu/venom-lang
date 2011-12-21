/** parser.yy Contains the Bison parser source */

%{ /*** C/C++ Declarations ***/

#include <stdio.h>
#include <string>
#include <vector>

#include <ast/expression/include.h>

%}

/*** yacc/bison Declarations ***/

/* Require bison 2.3 or later */
%require "2.3"

/* add debug output code to generated parser. disable this for release
 * versions. */
%debug

/* start symbol is named "start" */
%start start

/* write out a header file containing the token defines */
%defines

/* use newer C++ skeleton file */
%skeleton "lalr1.cc"

/* namespace to enclose parser in */
%name-prefix="venom"

/* set the parser's class identifier */
%define "parser_class_name" "Parser"

/* keep track of the current position within the input */
%locations
%initial-action
{
    // initialize the initial location object
    @$.begin.filename = @$.end.filename = &driver.streamname;
};

/* The driver is passed by reference to the parser and to the scanner. This
 * provides a simple but effective pure interface, not relying on global
 * variables. */
%parse-param { class Driver& driver }

/* verbose error messages */
%error-verbose

%union {
    int64_t         integerVal;
    double          doubleVal;
    std::string*    stringVal;

    ast::ASTNode* astNode;

    ast::ASTExpressionNode* expNode;
    ast::ExprNodeVec*       exprs;
    ast::DictPairVec*       pairs;
    ast::DictPair*          pair;
}

%token   END            0
%token   EOL

%token   IF             "if"
%token   ELSE           "else"
%token   ELSIF          "elsif"
%token   FOR            "for"
%token   WHILE          "while"
%token   DEF            "def"
%token   CLASS          "class"

%token   LSHIFT         "<<"
%token   RSHIFT         ">>"
%token   LE             "<="
%token   GE             ">="
%token   EQ             "=="
%token   NEQ            "!="
%token   AND            "and"
%token   OR             "or"
%token   NOT            "not"

%token   <integerVal>   INTEGER
%token   <doubleVal>    DOUBLE
%token   <stringVal>    STRING
%token   <stringVal>    IDENTIFIER

%type <astNode> start

%type <expNode> intlit doublelit strlit arraylit dictlit
                pairkey pairvalue variable expr literal atom
                unop_pm unop_bool binop_mult binop_add binop_shift
                binop_cmp binop_eq binop_bit_and binop_xor
                binop_bit_or binop_and binop_or

%type <exprs>   exprlist
%type <pairs>   pairlist
%type <pair>    pair

%{

#include "driver.h"
#include "scanner.h"

/* this "connects" the bison parser in the driver to the flex scanner class
 * object. it defines the yylex() function call to pull the next token from the
 * current lexer object of the driver context. */
#undef yylex
#define yylex driver.lexer->lex

%}

%% /*** Grammar Rules ***/

start  : /* empty */
       | start ';'
       | start EOL
       | start expr ';'
       | start expr EOL
       | start expr END

expr   : binop_or

literal : intlit
        | doublelit
        | strlit
        | arraylit
        | dictlit

atom : literal
     | variable
     | '(' expr ')' { $$ = $2; }

unop_pm : atom
        | '-' unop_pm { $$ = new ast::UnopNode($2, ast::UnopNode::MINUS); }
        | '+' unop_pm { $$ = new ast::UnopNode($2, ast::UnopNode::PLUS);  }

unop_bool : unop_pm
          | '~' unop_bool   { $$ = new ast::UnopNode($2, ast::UnopNode::BIT_NOT); }
          | "not" unop_bool { $$ = new ast::UnopNode($2, ast::UnopNode::CMP_NOT); }

binop_mult : unop_bool
           | binop_mult '*' unop_bool
             { $$ = new ast::BinopNode($1, $3, ast::BinopNode::MULT); }
           | binop_mult '/' unop_bool
             { $$ = new ast::BinopNode($1, $3, ast::BinopNode::DIV);  }
           | binop_mult '%' unop_bool
             { $$ = new ast::BinopNode($1, $3, ast::BinopNode::MOD);  }

binop_add : binop_mult
          | binop_add '+' binop_mult
            { $$ = new ast::BinopNode($1, $3, ast::BinopNode::ADD); }
          | binop_add '-' binop_mult
            { $$ = new ast::BinopNode($1, $3, ast::BinopNode::SUB); }

binop_shift : binop_add
            | binop_shift "<<" binop_add
              { $$ = new ast::BinopNode($1, $3, ast::BinopNode::BIT_LSHIFT); }
            | binop_shift ">>" binop_add
              { $$ = new ast::BinopNode($1, $3, ast::BinopNode::BIT_RSHIFT); }

binop_cmp : binop_shift
          | binop_cmp '<' binop_shift
            { $$ = new ast::BinopNode($1, $3, ast::BinopNode::CMP_LT); }
          | binop_cmp "<=" binop_shift
            { $$ = new ast::BinopNode($1, $3, ast::BinopNode::CMP_LE); }
          | binop_cmp '>' binop_shift
            { $$ = new ast::BinopNode($1, $3, ast::BinopNode::CMP_GT); }
          | binop_cmp ">=" binop_shift
            { $$ = new ast::BinopNode($1, $3, ast::BinopNode::CMP_GE); }

binop_eq : binop_cmp
         | binop_eq "==" binop_cmp
           { $$ = new ast::BinopNode($1, $3, ast::BinopNode::CMP_EQ);  }
         | binop_eq "!=" binop_cmp
           { $$ = new ast::BinopNode($1, $3, ast::BinopNode::CMP_NEQ); }

binop_bit_and : binop_eq
              | binop_bit_and '&' binop_eq
                { $$ = new ast::BinopNode($1, $3, ast::BinopNode::BIT_AND); }

binop_xor : binop_bit_and
          | binop_xor '^' binop_bit_and
            { $$ = new ast::BinopNode($1, $3, ast::BinopNode::BIT_XOR); }

binop_bit_or : binop_xor
             | binop_bit_or '|' binop_xor
               { $$ = new ast::BinopNode($1, $3, ast::BinopNode::BIT_OR); }

binop_and : binop_bit_or
          | binop_and "and" binop_bit_or
            { $$ = new ast::BinopNode($1, $3, ast::BinopNode::CMP_AND); }

binop_or : binop_and
         | binop_or "or" binop_and
           { $$ = new ast::BinopNode($1, $3, ast::BinopNode::CMP_OR); }

intlit : INTEGER { $$ = new ast::IntLiteralNode($1); }

doublelit : DOUBLE { $$ = new ast::DoubleLiteralNode($1); }

strlit : STRING { $$ = new ast::StringLiteralNode(*$1); delete $1; }

arraylit : '[' exprlist ']' { $$ = new ast::ArrayLiteralNode(*$2); delete $2; }

exprlist : /* empty */       { $$ = new ast::ExprNodeVec;  }
         | expr              { $$ = ast::MakeExprVec1($1); }
         | expr ',' exprlist { $3->push_back($1); $$ = $3; }

dictlit : '{' pairlist '}' { $$ = new ast::DictLiteralNode(*$2); delete $2; }

pairkey   : expr
pairvalue : expr

pair : pairkey ':' pairvalue { $$ = new ast::DictPair($1, $3); }

pairlist : /* empty */       { $$ = new ast::DictPairVec; }
         | pair              { $$ = ast::MakeDictPairVec1(*$1); delete $1; }
         | pair ',' pairlist { $3->push_back(*$1); $$ = $3; delete $1; }

variable : IDENTIFIER { $$ = new ast::VariableNode(*$1); delete $1; }

%% /*** Additional Code ***/

void venom::Parser::error(const Parser::location_type& l,
                          const std::string& m) {
    driver.error(l, m);
}
