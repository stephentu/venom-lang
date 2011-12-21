/** parser.yy Contains the Bison parser source */

%{ /*** C/C++ Declarations ***/

#include <stdio.h>
#include <string>
#include <vector>

#include "expression.h"

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
    CalcNode*       calcnode;
}

%token               END       0
%token               EOL
%token <integerVal>  INTEGER
%token <doubleVal>   DOUBLE
%token <stringVal>   STRING
%token <stringVal>   IDENTIFIER

%type <calcnode>  constant variable
%type <calcnode>  atomexpr powexpr unaryexpr mulexpr addexpr expr

%destructor { delete $$; } IDENTIFIER
%destructor { delete $$; } constant variable
%destructor { delete $$; } atomexpr powexpr unaryexpr mulexpr addexpr expr

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

constant : INTEGER
           {
         $$ = new CNConstant($1);
     }
         | DOUBLE
           {
         $$ = new CNConstant($1);
     }

variable : IDENTIFIER
           {
         if (!driver.calc.existsVariable(*$1)) {
       error(yyloc, std::string("Unknown variable \"") + *$1 + "\"");
       delete $1;
       YYERROR;
         }
         else {
       $$ = new CNConstant( driver.calc.getVariable(*$1) );
       delete $1;
         }
     }

atomexpr : constant
           {
         $$ = $1;
     }
         | variable
           {
         $$ = $1;
     }
         | '(' expr ')'
           {
         $$ = $2;
     }

powexpr  : atomexpr
          {
        $$ = $1;
    }
        | atomexpr '^' powexpr
          {
        $$ = new CNPower($1, $3);
    }

unaryexpr : powexpr
            {
    $$ = $1;
      }
          | '+' powexpr
            {
    $$ = $2;
      }
          | '-' powexpr
            {
    $$ = new CNNegate($2);
      }

mulexpr : unaryexpr
          {
        $$ = $1;
    }
        | mulexpr '*' unaryexpr
          {
        $$ = new CNMultiply($1, $3);
    }
        | mulexpr '/' unaryexpr
          {
        $$ = new CNDivide($1, $3);
    }
        | mulexpr '%' unaryexpr
          {
        $$ = new CNModulo($1, $3);
    }

addexpr : mulexpr
          {
        $$ = $1;
    }
        | addexpr '+' mulexpr
          {
        $$ = new CNAdd($1, $3);
    }
        | addexpr '-' mulexpr
          {
        $$ = new CNSubtract($1, $3);
    }

expr  : addexpr
          {
        $$ = $1;
    }

assignment : IDENTIFIER '=' expr
             {
     driver.calc.variables[*$1] = $3->evaluate();
     std::cout << "Setting variable " << *$1
         << " = " << driver.calc.variables[*$1] << "\n";
     delete $1;
     delete $3;
       }

start  : /* empty */
        | start ';'
        | start EOL
  | start assignment ';'
  | start assignment EOL
  | start assignment END
        | start expr ';'
          {
        driver.calc.expressions.push_back($2);
    }
        | start expr EOL
          {
        driver.calc.expressions.push_back($2);
    }
        | start expr END
          {
        driver.calc.expressions.push_back($2);
    }

%% /*** Additional Code ***/

void venom::Parser::error(const Parser::location_type& l, const std::string& m) {
    driver.error(l, m);
}
