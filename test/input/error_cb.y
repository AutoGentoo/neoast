%include {
#include <stdlib.h>
#include <stddef.h>
}

%top {
void lexer_error_cb(void* ctx,
                    const char* input,
                    const TokenPosition* position,
                    const char* state_name);

void parser_error_cb(void* ctx,
                     const char* const* token_names,
                     const TokenPosition* position,
                     uint32_t last_token,
                     uint32_t current_token,
                     const uint32_t expected_tokens[],
                     uint32_t expected_tokens_n);

}

%option prefix="error"
%option parsing_error_cb="parser_error_cb"
%option lexing_error_cb="lexer_error_cb"
%option annotate_line="FALSE"

%union {
    int out;
}
%left '+'
%left '-'
%right '*'
%right '/'

%token<out> A_TOKEN
%token '+' '-' '*' '/'
%type<out> out
%type<out> expr
%start<out> out

==
"[ \n\r]+"      { }
"[0-9]+"    { yyval->out = strtol(yytext, NULL, 10); return A_TOKEN; }
"\+"        { return '+'; }
"-"         { return '-'; }
"\*"        { return '*'; }
"/"         { return '/'; }
==

%%

out: expr           { $$ = $1; }
    ;

expr: A_TOKEN       { $$ = $1; }
    | expr '+' expr { $$ = $1 + $3; }
    | expr '-' expr { $$ = $1 - $3; }
    | expr '*' expr { $$ = $1 * $3; }
    | expr '/' expr { $$ = $1 / $3; }
    ;

%%
