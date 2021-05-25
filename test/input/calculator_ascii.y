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
%option prefix="calc_ascii"

%union {
    double number;
}

%token<number> TOK_N
%token '+'
%token '-'
%token '/'
%token '*'
%token '^'
%token '('
%token ')'

%type <number> expr
%type <number> expr_first
%type <number> program
%start <number> program

// Test global comment

%left '+'
%left '-'
%left '*'
%left '/'
%right '^'

==
// Test lex rule comment
"[ ]+"        {return -1;}
"[0-9]+"      {yyval->number = strtod(yytext, NULL); return TOK_N;}
"\+"          {return '+';}
"\-"          {return '-';}
"\/"          {return '/';}
"\*"          {return '*';}
"\^"          {return '^';}
"\("          {return '(';}
"\)"          {return ')';}

==

%%
// Test grammar rule comment
program: expr     {$$ = $1;}
     |            {$$ = 0;}
     ;

expr_first:
      TOK_N                {$$ = $1;}
    | expr '/' expr        {$$ = $1 / $3;}
    | expr '*' expr        {$$ = $1 * $3;}
    | expr '^' expr        {$$ = pow($1, $3);}
    | '(' expr ')'         {$$ = $2;}
    ;

expr:
      expr_first                { $$ = $1; }
    | expr '+' expr             {$$ = $1 + $3;}
    | expr '-' expr             {$$ = $1 - $3;}
    ;
%%