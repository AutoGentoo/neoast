%top {

#include <stdlib.h>
#include <codegen/codegen.h>
#include <codegen/compiler.h>
#include <stddef.h>
#include <string.h>
#include <stdint.h>
#include <assert.h>

void lexing_error_cb(const char* input,
                     const TokenPosition* position,
                     const char* lexer_state);

void parsing_error_cb(const char* const* token_names,
                      const TokenPosition* position,
                      uint32_t last_token,
                      uint32_t current_token,
                      const uint32_t expected_tokens[],
                      uint32_t expected_tokens_n);

struct LexerTextBuffer
{
    int32_t counter;
    uint32_t s;
    uint32_t n;
    char* buffer;
    TokenPosition start_position;
};

static __thread struct LexerTextBuffer brace_buffer = {0};

static inline void ll_add_to_brace(const char* lex_text, uint32_t len);
static inline void ll_match_brace(const TokenPosition* start_position);

}

%bottom {

static inline void ll_add_to_brace(const char* lex_text, uint32_t len)
{
    if (len + brace_buffer.n >= brace_buffer.s)
    {
        brace_buffer.s *= 2;
        brace_buffer.buffer = realloc(brace_buffer.buffer, brace_buffer.s);
    }

    strncpy(brace_buffer.buffer + brace_buffer.n, lex_text, len);
    brace_buffer.n += len;
}

static inline void ll_match_brace(const TokenPosition* start_position)
{
    brace_buffer.s = 1024;
    brace_buffer.buffer = malloc(brace_buffer.s);
    brace_buffer.n = 0;
    brace_buffer.start_position = *start_position;
    brace_buffer.counter = 1;
}

}

%option parser_type="LALR(1)"
%option prefix="cc"
%option debug_table="FALSE"
%option annotate_line="FALSE"
%option lexing_error_cb="lexing_error_cb"
%option parsing_error_cb="parsing_error_cb"

%union {
    char* identifier;
    char ascii;
    key_val_t key_val_type;
    KeyVal* key_val;
    struct LexerRuleProto* l_rule;
    struct Token* token;
    struct GrammarRuleSingleProto* g_single_rule;
    struct GrammarRuleProto* g_rule;
    struct File* file;
    struct { char* string; TokenPosition position; } action;
}

%token LL
%token GG
%token OPTION
%token START
%token UNION
%token TYPE
%token LEFT
%token RIGHT
%token TOP
%token INCLUDE
%token END_STATE
%token<key_val> MACRO
%token BOTTOM
%token TOKEN // %token
%token DESTRUCTOR
%token<ascii> ASCII
%token<identifier> LITERAL
%token<action> ACTION
%token<identifier> IDENTIFIER
%token<identifier> LEX_STATE
%token '<'
%token '>'
%token ':'
%token '|'
%token ';'
%token '='
%type<g_single_rule> single_grammar
%type<g_single_rule> multi_grammar
%type<g_rule> grammar
%type<g_rule> grammars

%type<key_val> header
%type<key_val> pair
%type<l_rule> lexer_rules
%type<l_rule> lexer_rules_state
%type<l_rule> lexer_rule
%type<token> tokens
%type<token> token

%type <file> program
%start <file> program

%destructor<identifier>     { free($$); }
%destructor<action>         { free($$.string); }
%destructor<l_rule>         { lexer_rule_free($$); }
%destructor<g_single_rule>  { grammar_rule_single_free($$); }
%destructor<g_rule>         { grammar_rule_multi_free($$); }
%destructor<key_val>        { key_val_free($$); }
%destructor<token>          { tokens_free($$); }
%destructor<file>           { file_free($$); }

+literal        \"(\\.|[^\"\\])*\"
+identifier     [A-Za-z_][\w]*
+lex_state      <[A-Za-z_][\w]*>[\s]*\{
+ascii          '([\x20-\x7E]|\\.)'
+macro          \+{identifier}[ ][\s]*[^\n]+
+whitespace     [ \t\r\n]+

==

// Initial state

// Whitespace
"{whitespace}"      { /* skip */ }
"//[^\n]*"          { /* skip */ }
"/\*"               { yypush(S_COMMENT); }

// These take precedence over literal tokens
"{literal}"         { yyval->identifier = strndup(yytext + 1, yylen - 2); return LITERAL; }
"{ascii}"           { yyval->ascii = yytext[1]; return ASCII; }

// Don't build anything during lexing to keep things simple
// This is different from the way the bootstraping compiler does it
"=="                { yypush(S_LL_RULES); return LL; }
"%%"                { yypush(S_GG_RULES); return GG; }
"="                 { return '='; }
"<"                 { return '<'; }
">"                 { return '>'; }
"%option"           { return OPTION; }
"%token"            { return TOKEN; }
"%start"            { return START; }
"%union"            { return UNION; }
"%type"             { return TYPE; }
"%left"             { return LEFT; }
"%right"            { return RIGHT; }
"%top"              { return TOP; }
"%include"          { return INCLUDE; }
"%bottom"           { return BOTTOM; }
"%destructor"       { return DESTRUCTOR; }
"{macro}"           {
                        // Find the white space delimiter
                        const char* split = strchr(yytext + 1, ' ');
                        assert(split);

                        char* key = strndup(yytext + 1, split - yytext - 1);

                        // Find the start of the regex rule
                        while (*split == ' ') split++;

                        char* value = strdup(split);
                        yyval->key_val = key_val_build(yyposition, KEY_VAL_MACRO, key, value);
                        return MACRO;
                    }
"{identifier}"      { yyval->identifier = strdup(yytext); return IDENTIFIER; }
"\{"                { yypush(S_MATCH_BRACE); ll_match_brace(yyposition); }

<S_LL_RULES> {
"[\n]"              { /* skip */ }
"{whitespace}"      { /* skip */ }
"//[^\n]*"          { /* skip */ }
"/\*"               { yypush(S_COMMENT); }
"=="                { yypop(); return LL; }
"{lex_state}"       {
                        const char* start_ptr = strchr(yytext, '<');
                        const char* end_ptr = strchr(yytext, '>');
                        yyval->identifier = strndup(start_ptr + 1, end_ptr - start_ptr - 1);
                        yypush(S_LL_STATE);

                        return LEX_STATE;
                    }
"{literal}"         { yyval->identifier = strndup(yytext + 1, yylen - 2); return LITERAL; }
"\{"                 { yypush(S_MATCH_BRACE); ll_match_brace(yyposition); }
}

<S_LL_STATE> {
"{whitespace}"      { /* skip */ }
"//[^\n]*"          { /* skip */ }
"/\*"               { yypush(S_COMMENT); }

"{literal}"         { yyval->identifier = strndup(yytext + 1, yylen - 2); return LITERAL; }

"\{"                 { yypush(S_MATCH_BRACE); ll_match_brace(yyposition); }
"\}"                 { yypop(); return END_STATE; }

}

<S_GG_RULES> {
"{whitespace}"      { /* skip */ }
"//[^\n]*"          { /* skip */ }
"{ascii}"           { yyval->ascii = yytext[1]; return ASCII; }
"/\*"               { yypush(S_COMMENT); }
"%%"                { yypop(); return GG; }
"{identifier}"      { yyval->identifier = strdup(yytext); return IDENTIFIER; }
":"                 { return ':'; }
"\|"                { return '|'; }
";"                 { return ';'; }
"\{"                 { yypush(S_MATCH_BRACE); ll_match_brace(yyposition); }
}

<S_MATCH_BRACE> {
// TODO: Comments inside braces should not be ignored because we want
//       codegen to paste these in
"/\*"               { yypush(S_COMMENT); }

// ASCII literals
"{ascii}"           { ll_add_to_brace(yytext, yylen); }

// String literals
"\"(\\.|[^\"\\])*\"" { ll_add_to_brace(yytext, yylen); }

// Everything else
"[^\}\{\"\'/]+"      { ll_add_to_brace(yytext, yylen); }
"/"                  { ll_add_to_brace(yytext, yylen); }

"\{"                { brace_buffer.counter++; brace_buffer.buffer[brace_buffer.n++] = '{'; }
"\}"                {
                        brace_buffer.counter--;
                        if (brace_buffer.counter <= 0)
                        {
                            // This is the outer-most brace
                            brace_buffer.buffer[brace_buffer.n++] = 0;
                            yypop();
                            yyval->action.string = strndup(brace_buffer.buffer, brace_buffer.n);
                            yyval->action.position = brace_buffer.start_position;
                            free(brace_buffer.buffer);
                            brace_buffer.buffer = NULL;
                            return ACTION;
                        }
                        else
                        {
                            brace_buffer.buffer[brace_buffer.n++] = '}';
                        }
                    }
}

<S_COMMENT> {
"[^\*]+"            { /* Absorb comment */ }
"\*/"               { yypop(); }
"\*"                { }
}

==

%%

token: IDENTIFIER                   { $$ = build_token($p1, $1); }
     | ASCII                        { $$ = build_token_ascii($p1, $1); }
     ;

tokens: token                       { $$ = $1; }
      | token tokens                { $$ = $1; $$->next = $2; }
      ;

pair: OPTION IDENTIFIER '=' LITERAL         { $$ = declare_option($p1, $2, $4); }
    | TOKEN tokens                          { $$ = declare_tokens($2); }
    | START '<' IDENTIFIER '>' IDENTIFIER   { $$ = declare_start($p1, $3, $5); }
    | TOKEN '<' IDENTIFIER '>' tokens       { $$ = declare_typed_tokens($3, $5); }
    | TYPE '<' IDENTIFIER '>' tokens        { $$ = declare_types($3, $5); }
    | DESTRUCTOR '<' IDENTIFIER '>' ACTION  { $$ = declare_destructor(&$5.position, $3, $5.string); }
    | RIGHT tokens                          { $$ = declare_right($2); }
    | LEFT tokens                           { $$ = declare_left($2); }
    | TOP ACTION                            { $$ = declare_top(&$2.position, $2.string); }
    | INCLUDE ACTION                        { $$ = declare_include(&$2.position, $2.string); }
    | BOTTOM ACTION                         { $$ = declare_bottom(&$2.position, $2.string); }
    | UNION ACTION                          { $$ = declare_union(&$2.position, $2.string); }
    | MACRO                                 { $$ = $1; }
    ;

header: pair                        { $$ = $1; }
      | pair header                 { $$ = $1->back ? $1->back : $1; $1->next = $2; }
      ;

lexer_rule: LITERAL ACTION          { $$ = declare_lexer_rule(&$2.position, $1, $2.string); }
          | LEX_STATE lexer_rules_state END_STATE { $$ = declare_state_rule($p1, $1, $2); }
          ;

lexer_rules_state: lexer_rule                   { $$ = $1; }
                 | lexer_rule lexer_rules_state { $$ = $1; $$->next = $2; }
                 ;
lexer_rules: lexer_rule             { $$ = $1; }
           | lexer_rule lexer_rules { $$ = $1; $$->next = $2; }
           ;

single_grammar: tokens ACTION                   { $$ = declare_single_grammar(&$2.position, $1, $2.string); }

              // Empty rule
              | ACTION                          { $$ = declare_single_grammar(&$1.position, NULL, $1.string); }

              // Default action
              | tokens                          { $$ = declare_single_grammar($p1, $1, calloc(1, 1)); }
              ;

multi_grammar: single_grammar                   { $$ = $1; }
             | single_grammar '|' multi_grammar { $$ = $1; $$->next = $3; }
             ;

grammar: IDENTIFIER ':' multi_grammar ';'       { $$ = declare_grammar($p1, $1, $3); }
       ;


grammars: grammar                   { $$ = $1; }
        | grammar grammars          { $$ = $1; $$->next = $2; }
        ;

program: header LL lexer_rules LL GG grammars GG { $$ = declare_file($1, $3, $6); }
       ;

%%
