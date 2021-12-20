%include {
#include <stdio.h>
#include <stdlib.h>
#include <math.h>
}

%top {
// This is a calculator test program
}

// This is default, just want to test the parser
%option parser_type="LALR(1)"
%option debug_table="TRUE"
%option annotate_line="TRUE"
%option debug_ids="$d+-/*^()ES"
%option prefix="calc_ascii"
%option graphviz_file="g.viz"

%token<number> TOK_N
%token '+' '-' '/' '*'
       '^' '(' ')'

%type <number> expr program
%start <number> program

// Test global comment

%left '+' '-' '*' '/'
%right '^'

%union {
    double number;
}

/*
 * Block comment
 */

==
// Test lex rule comment
"[ ]+"        { }
"[0-9]+"      {yyval->number = strtod(yytext, NULL); return TOK_N;}
"\+"          {return '+';}
"\-"          {return '-';}
"\/"          {return '/';}
"\*"          {return '*';}
"\^"          {return '^';}
"\("          {return '(';}
"\)"          {return ')';}


/*
Block comment
*/

==

%%
// Test grammar rule comment
program: expr     {$$ = $1;}
     |            {$$ = 0;}
     ;

expr: TOK_N              {$$ = $1;}
    | expr '+' expr      {$$ = $1 + $3;}
    | expr '-' expr      {$$ = $1 - $3;}
    | expr '/' expr      {$$ = $1 / $3;}
    | expr '*' expr      {$$ = $1 * $3;}
    | expr '^' expr      {$$ = pow($1, $3);}
    | '(' expr ')'       {$$ = $2;}
    ;


/*
Block comment
*/
%%
