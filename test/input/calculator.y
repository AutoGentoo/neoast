%top {
#include <stdio.h>
#include <stdlib.h>
#include <math.h>

// This is a calculator test program
}

// This is default, just want to test the parser
%option parser_type="LALR(1)"
//%option disable_locks="TRUE"
%option debug_table="TRUE"
%option debug_ids="$d+-/*^()ES"
%option prefix="calc"

%token<number> TOK_N
%token TOK_PLUS
%token TOK_MINUS
%token TOK_SLASH
%token TOK_STAR
%token TOK_CARET
%token TOK_P_OPEN
%token TOK_P_CLOSE

%type <number> expr
%type <number> program
%start <number> program

// Test global comment

%left TOK_PLUS
%left TOK_MINUS
%left TOK_STAR
%left TOK_SLASH
%right TOK_CARET

%union {
    double number;
}

/*
 * Block comment
 */

==
// Test lex rule comment
"[ ]+"         {return -1;}
"[0-9]+"       {yyval->number = strtod(yytext, NULL); return TOK_N;}
"\\+"          {return TOK_PLUS;}
"\\-"          {return TOK_MINUS;}
"\\/"          {return TOK_SLASH;}
"\\*"          {return TOK_STAR;}
"\\^"          {return TOK_CARET;}
"\\("          {return TOK_P_OPEN;}
"\\)"          {return TOK_P_CLOSE;}


/*
Block comment
*/

==

%%
// Test grammar rule comment
program: expr     {$$ = $1;}
     |            {$$ = 0;}
     ;

expr: TOK_N                             {$$ = $1;}
    | expr TOK_PLUS expr                {$$ = $1 + $3;}
    | expr TOK_MINUS expr               {$$ = $1 - $3;}
    | expr TOK_SLASH expr               {$$ = $1 / $3;}
    | expr TOK_STAR expr                {$$ = $1 * $3;}
    | expr TOK_CARET expr               {$$ = pow($1, $3);}
    | TOK_P_OPEN expr TOK_P_CLOSE       {$$ = $2;}
    ;


/*
Block comment
*/
%%