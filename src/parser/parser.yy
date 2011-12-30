/** parser.yy Contains the Bison parser source */

%{ /*** C/C++ Declarations ***/

#include <cassert>
#include <cstdio>

#include <string>
#include <vector>

#include <ast/include.h>
#include <util/stl.h>

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
    int64_t                       integerVal;
    double                        doubleVal;
    std::string*                  stringVal;

    ast::ASTStatementNode*        stmtNode;
    ast::ASTExpressionNode*       expNode;

    ast::StmtNodeVec*             stmts;
    ast::ExprNodeVec*             exprs;

    ast::DictPairVec*             pairs;
    ast::DictPair*                pair;

    ast::ParameterizedTypeString* typeString;
    ast::TypeStringVec*           typeStrVec;

    util::StrVec*                 strVec;
}

%token   END            0
%token   EOL

%token   IF             "if"
%token   THEN           "then"
%token   ELSE           "else"
%token   ELSIF          "elsif"
%token   FOR            "for"
%token   WHILE          "while"
%token   DO             "do"
%token   DEF            "def"
%token   ENDTOK         "end"
%token   CLASS          "class"
%token   ATTR           "attr"
%token   RETURN         "return"
%token   SELF           "self"
%token   SUPER          "super"
%token   IMPORT         "import"
%token   TRUE           "True"
%token   FALSE          "False"
%token   NIL            "Nil"

%token   LSHIFT         "<<"
%token   RSHIFT         ">>"
%token   LE             "<="
%token   GE             ">="
%token   EQ             "=="
%token   NEQ            "!="
%token   AND            "and"
%token   OR             "or"
%token   NOT            "not"

%token   COLONCOLON     "::"
%token   LEFTARROW      "<-"
%token   RIGHTARROW     "->"

%token   <integerVal>   INTEGER
%token   <doubleVal>    DOUBLE
%token   <stringVal>    STRING
%token   <stringVal>    IDENTIFIER

%type <stmtNode>   start stmt stmtlist stmtexpr assignstmt ifstmt ifstmt_else
                   whilestmt forstmt returnstmt funcdeclstmt ctordeclstmt classdeclstmt
                   classbodystmt classbodystmtlist attrdeclstmt

%type <expNode>    intlit boollit nillit doublelit strlit arraylit dictlit
                   pairkey pairvalue variable typedvariable expr literal atom
                   self super primary unop_pm unop_bool binop_mult binop_add
                   binop_shift binop_cmp binop_eq binop_bit_and binop_xor
                   binop_bit_or binop_and binop_or attropteq

%type <stmts>      stmtlist_buffer classbodystmtlist_buffer
%type <exprs>      exprlist paramlist paramlist0
%type <pairs>      pairlist
%type <pair>       pair

%type <typeString> paramtypename rettype
%type <typeStrVec> paramtypenames paramtypenames0 optparamtypenames inheritance

%type <strVec>     typename typename0 typeparams typeparams0 typeparams0rest

%{

#include <parser/driver.h>
#include <parser/scanner.h>

/* this "connects" the bison parser in the driver to the flex scanner class
 * object. it defines the yylex() function call to pull the next token from the
 * current lexer object of the driver context. */
#undef yylex
#define yylex driver.lexer->lex

%}

%% /*** Grammar Rules ***/

start  : stmtlist END { driver.ctx.stmts = $1; $$ = $1; }

stmtlist : stmtlist_buffer { $$ = new ast::StmtListNode(*$1); delete $1; }

stmtlist_buffer : /* empty */          { $$ = new ast::StmtNodeVec;  }
                | stmtlist_buffer stmt { $1->push_back($2); $$ = $1; }

stmt   : stmtexpr
       | assignstmt
       | ifstmt
       | whilestmt
       | forstmt
       | returnstmt
       | funcdeclstmt
       | classdeclstmt

stmtexpr : expr exprend { $$ = new ast::StmtExprNode($1); }

exprend  : ';'

assignstmt : expr          '=' expr exprend { $$ = new ast::AssignNode($1, $3); }
           | typedvariable '=' expr exprend { $$ = new ast::AssignNode($1, $3); }

ifstmt : "if" expr "then" stmtlist ifstmt_else "end"
         { $$ = new ast::IfStmtNode($2, $4, $5); }

ifstmt_else : /* empty */
              { $$ = new ast::StmtListNode(ast::StmtNodeVec()); }
            | "elsif" expr "then" stmtlist ifstmt_else
              { $$ = new ast::IfStmtNode($2, $4, $5); }
            | "else" stmtlist
              { $$ = $2; }

whilestmt : "while" expr "do" stmtlist "end"
            { $$ = new ast::WhileStmtNode($2, $4); }

forstmt : "for" variable "<-" expr "do" stmtlist "end"
           { $$ = new ast::ForStmtNode($2, $4, $6); }

returnstmt : "return" exprend      { $$ = new ast::ReturnNode(NULL); }
           | "return" expr exprend { $$ = new ast::ReturnNode($2);   }

funcdeclstmt : "def" IDENTIFIER typeparams '(' paramlist ')' rettype '=' stmtlist "end"
               {
                 $$ = new ast::FuncDeclNode(*$2, *$3, *$5, $7, $9);
                 delete $2; delete $3; delete $5;
               }

ctordeclstmt : "def" "self" '(' paramlist ')' '=' stmtlist "end"
                {
                  $$ = new ast::CtorDeclNode(*$4, $7, ast::ExprNodeVec()); delete $4;
                }
             | "def" "self" '(' paramlist ')' ':' "super" '(' exprlist ')' '=' stmtlist "end"
                {
                  $$ = new ast::CtorDeclNode(*$4, $12, *$9); delete $4; delete $9;
                }

classdeclstmt : "class" IDENTIFIER typeparams inheritance classbodystmtlist "end"
                {
                  $$ = new ast::ClassDeclNode(*$2, *$4, *$3, $5);
                  delete $2; delete $3; delete $4;
                }

attrdeclstmt : "attr" variable '=' expr
             { $$ = new ast::ClassAttrDeclNode($2, $4);  }
             | "attr" typedvariable attropteq
             { $$ = new ast::ClassAttrDeclNode($2, $3);  }

attropteq : /* empty */ { $$ = NULL; }
          | '=' expr    { $$ = $2;   }

classbodystmt : funcdeclstmt
              | ctordeclstmt
              | classdeclstmt
              | attrdeclstmt

classbodystmtlist : classbodystmtlist_buffer
                    { $$ = new ast::StmtListNode(*$1); delete $1; }

classbodystmtlist_buffer : /* empty */
                           { $$ = new ast::StmtNodeVec;  }
                         | classbodystmtlist_buffer classbodystmt
                           { $1->push_back($2); $$ = $1; }

typeparams : /* empty */ { $$ = new util::StrVec; }
           | '{' typeparams0 '}' { $$ = $2; }

typeparams0: typeparams0rest IDENTIFIER { $1->push_back(*$2); $$ = $1; delete $2; }

typeparams0rest : /* empty */ { $$ = new util::StrVec; }
                | typeparams0rest IDENTIFIER ',' { $1->push_back(*$2); $$ = $1; delete $2; }

inheritance : /* empty */         { $$ = new ast::TypeStringVec; }
            | "<-" paramtypenames { $$ = $2; }


rettype : /* empty */        { $$ = NULL; }
        | "->" paramtypename { $$ = $2;   }

paramlist : /* empty */
            { $$ = new ast::ExprNodeVec;  }
          | paramlist0 typedvariable
            { $1->push_back($2); $$ = $1; }

paramlist0 : /* empty */
             { $$ = new ast::ExprNodeVec;  }
           | paramlist0 typedvariable ','
             { $1->push_back($2); $$ = $1; }

expr   : binop_or

literal : intlit
        | boollit
        | nillit
        | doublelit
        | strlit
        | arraylit
        | dictlit

atom : literal
     | variable
     | self
     | super
     | '(' expr ')' { $$ = $2; }

primary : atom
        | primary '.' IDENTIFIER
          { $$ = new ast::AttrAccessNode($1, *$3); delete $3; }
        | primary '[' expr ']'
          { $$ = new ast::ArrayAccessNode($1, $3); }
        | primary optparamtypenames '(' exprlist ')'
          {
            $$ = new ast::FunctionCallNode($1, *$2, *$4);
            delete $2; delete $4;
          }

unop_pm : primary
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

boollit : TRUE  { $$ = new ast::BoolLiteralNode(true);  }
        | FALSE { $$ = new ast::BoolLiteralNode(false); }

nillit: NIL { $$ = new ast::NilLiteralNode; }

doublelit : DOUBLE { $$ = new ast::DoubleLiteralNode($1); }

strlit : STRING { $$ = new ast::StringLiteralNode(*$1); delete $1; }

arraylit : '[' exprlist ']' { $$ = new ast::ArrayLiteralNode(*$2); delete $2; }

exprlist : /* empty */       { $$ = new ast::ExprNodeVec;  }
         | expr              { $$ = ast::MakeExprVec1($1); }
         | exprlist ',' expr { $1->push_back($3); $$ = $1; }

dictlit : '{' pairlist '}' { $$ = new ast::DictLiteralNode(*$2); delete $2; }

pairkey   : expr
pairvalue : expr

pair : pairkey ':' pairvalue { $$ = new ast::DictPair($1, $3); }

pairlist : /* empty */       { $$ = new ast::DictPairVec;      }
         | pair              { $$ = ast::MakeDictPairVec1($1); }
         | pairlist ',' pair { $1->push_back($3); $$ = $1;     }

variable : IDENTIFIER { $$ = new ast::VariableNode(*$1, NULL); delete $1; }

typedvariable : IDENTIFIER "::" paramtypename
                { $$ = new ast::VariableNode(*$1, $3); delete $1; }

paramtypename : typename optparamtypenames
                {
                  $$ = new ast::ParameterizedTypeString(*$1, *$2);
                  delete $1; delete $2;
                }

optparamtypenames : /* empty */            { $$ = new ast::TypeStringVec; }
                  | '{' paramtypenames '}' { $$ = $2;                     }

paramtypenames : paramtypenames0 paramtypename { $1->push_back($2); $$ = $1; }

paramtypenames0 : /* empty */
                  { $$ = new ast::TypeStringVec; }
                | paramtypenames0 paramtypename ','
                  { $1->push_back($2); $$ = $1;  }

typename : typename0 IDENTIFIER
           {
              $1->push_back(*$2); $$ = $1; delete $2;
           }

typename0 : /* empty */
           { $$ = new util::StrVec; }
          | typename0 IDENTIFIER '.'
           { $1->push_back(*$2); $$ = $1; delete $2; }

self     : "self" { $$ = new ast::VariableSelfNode; }

super    : "super" { $$ = new ast::VariableSuperNode; }

%% /*** Additional Code ***/

void venom::Parser::error(const Parser::location_type& l,
                          const std::string& m) {
    driver.error(l, m);
}
