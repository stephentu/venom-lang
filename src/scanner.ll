/** scanner.ll Define the Flex lexical scanner */

%{ /*** C/C++ Declarations ***/

#include <string>

#include "scanner.h"

/* import the parser's token type into a local typedef */
typedef venom::Parser::token      token;
typedef venom::Parser::token_type token_type;

/* By default yylex returns int, we use token_type. Unfortunately yyterminate
 * by default returns 0, which is not of token_type. */
#define yyterminate() return token::END

/* This disables inclusion of unistd.h, which is not available under Visual C++
 * on Win32. The C++ scanner uses STL streams instead. */
#define YY_NO_UNISTD_H

%}

/*** Flex Declarations and Options ***/

/* enable c++ scanner class generation */
%option c++

/* change the name of the scanner class. results in "VenomFlexLexer" */
%option prefix="Venom"

/* the manual says "somewhat more optimized" */
%option batch

/* enable scanner to generate debug output. disable this for release
 * versions. */
%option debug

/* no support for include files is planned */
%option yywrap nounput

/* enables the use of start condition stacks */
%option stack

/* The following paragraph suffices to track locations accurately. Each time
 * yylex is invoked, the begin position is moved onto the end position. */
%{
#define YY_USER_ACTION  yylloc->columns(yyleng);
%}

DIGIT [0-9]
OCTDIGIT [0-7]
HEXDIGIT ([0-9]|[a-f]|[A-F])

POINTFLOAT ([0-9]*"."[0-9]+)|([0-9]+".")
EXPFLOAT ([0-9]|{POINTFLOAT})[eE][+-]?[0-9]+

IDENTIFIER [a-zA-Z][a-zA-Z0-9_]*

STRCHAR ([^\n\\"]|\\.)
LSTRCHAR [^\n']

%% /*** Tokens part ***/

 /* code to place at the beginning of yylex() */
%{
    // reset location
    yylloc->step();
%}

 /* Keywords */
"if"     { return token::IF;     }
"then"   { return token::THEN;   }
"else"   { return token::ELSE;   }
"elsif"  { return token::ELSIF;  }
"for"    { return token::FOR;    }
"while"  { return token::WHILE;  }
"do"     { return token::DO;     }
"def"    { return token::DEF;    }
 /* Must be ENDTOK since token::END is already taken */
"end"    { return token::ENDTOK; }
"class"  { return token::CLASS;  }
"attr"   { return token::ATTR;   }
"return" { return token::RETURN; }
"import" { return token::IMPORT; }
"self"   { return token::SELF;   }

 /* Operator tokens */
"<<"     { return token::LSHIFT; }
">>"     { return token::RSHIFT; }
"<="     { return token::LE;     }
">="     { return token::GE;     }
"=="     { return token::EQ;     }
"!="     { return token::NEQ;    }
"and"    { return token::AND;    }
"or"     { return token::OR;     }
"not"    { return token::NOT;    }

 /* Misc tokens */
"::"     { return token::COLONCOLON; }
"<-"     { return token::LEFTARROW;  }
"->"     { return token::RIGHTARROW; }

"#"[^\n]* {
    yylloc->step();
}

0{OCTDIGIT}+ {
    yylval->integerVal = strtol(yytext, NULL, 8);
    return token::INTEGER;
}

{DIGIT}+ {
    yylval->integerVal = atoi(yytext);
    return token::INTEGER;
}

0[xX]{HEXDIGIT}+ {
    yylval->integerVal = strtol(yytext, NULL, 16);
    return token::INTEGER;
}

{POINTFLOAT}|{EXPFLOAT} {
    yylval->doubleVal = strtod(yytext, NULL);
    return token::DOUBLE;
}

\"{STRCHAR}*\" {
    yylval->stringVal = new std::string(yytext + 1, yyleng - 2);
    return token::STRING;
}

'{LSTRCHAR}*' {
    yylval->stringVal = new std::string(yytext + 1, yyleng - 2);
    return token::STRING;
}

{IDENTIFIER} {
    yylval->stringVal = new std::string(yytext, yyleng);
    return token::IDENTIFIER;
}

 /* gobble up white-spaces */
[ \t\r]+ {
    yylloc->step();
}

 /* gobble up end-of-lines */
\n {
    yylloc->lines(yyleng); yylloc->step();
    //return token::EOL;
}

 /* pass all other characters up to bison */
. {
    return static_cast<token_type>(*yytext);
}

%% /*** Additional Code ***/

namespace venom {

Scanner::Scanner(std::istream* in, std::ostream* out)
    : VenomFlexLexer(in, out) {}

Scanner::~Scanner() {}

void Scanner::set_debug(bool b) { yy_flex_debug = b; }

}

/* This implementation of VenomFlexLexer::yylex() is required to fill the
 * vtable of the class VenomFlexLexer. We define the scanner's main yylex
 * function via YY_DECL to reside in the Scanner class instead. */

#ifdef yylex
#undef yylex
#endif

int VenomFlexLexer::yylex() { return 0; }

/* When the scanner receives an end-of-file indication from YY_INPUT, it then
 * checks the yywrap() function. If yywrap() returns false (zero), then it is
 * assumed that the function has gone ahead and set up `yyin' to point to
 * another input file, and scanning continues. If it returns true (non-zero),
 * then the scanner terminates, returning 0 to its caller. */

int VenomFlexLexer::yywrap() { return 1; }

/* vim: set ft=lex : */
