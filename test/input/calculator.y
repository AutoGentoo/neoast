%top {
#include <stdio.h>
#include <math.h>

// This is a calculator test program
}

%token<integer> TOK_N
%token TOK_PLUS
%token TOK_MINUS
%token TOK_SLASH
%token TOK_STAR
%token TOK_CARET
%token TOK_P_OPEN
%token TOK_P_CLOSE

%type <number> expr
%type <number> start
%start program

%left TOK_PLUS
%left TOK_MINUS
%left TOK_STAR
%left TOK_SLASH
%right TOK_CARET

%union {
    double number;
}

==

"[0-9]+"      {yyval->integer = strtod(yytext, NULL, 10); return TOK_N;}
"\\+"          {return TOK_PLUS;}
"\\-"          {return TOK_MINUS;}
"\\/"          {return TOK_SLASH;}
"\\*"          {return TOK_STAR;}
"\\^"          {return TOK_CARET;}
"\\("          {return TOK_P_OPEN;}
"\\)"          {return TOK_P_CLOSE;}

==

%%

program: expr     {$$ = $1;}
     |  MT      {$$ = 0;}
     ;

expr: TOK_N                             {$$ = $1}
    | expr TOK_PLUS expr                {$$ = $1 + $3;}
    | expr TOK_MINUS expr               {$$ = $1 - $3;}
    | expr TOK_SLASH expr               {$$ = $1 / $3;}
    | expr TOK_STAR expr                {$$ = $1 * $3;}
    | expr TOK_CARET expr               {$$ = $1 ^ $3;}
    | TOK_P_OPEN expr TOK_P_CLOSE       {$$ = $2;}
    ;
%%