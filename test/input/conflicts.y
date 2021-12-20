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

// To generate conflicts, we will not put precedence on ambiguous operators
//%left '+'
//%left '-'
//%right '*'
//%right '/'

%token<out> id
%token ','
%type optcomma
%type<out> list
%start<out> list

==
"[ \n\r]+"      { }
==

%%

list    : id
        | list optcomma id
        | list ','               {yyerror ("extra comma ignored");}
        ;
optcomma: ','
        |                        {yyerror ("missing comma inserted");}
        ;


%%
