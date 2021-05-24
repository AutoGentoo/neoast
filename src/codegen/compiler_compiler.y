%top {

#include <stdlib.h>
#include <codegen/codegen.h>
#include <codegen/compiler.h>
#include <stddef.h>
#include <string.h>
#include <stdint.h>
#include <assert.h>

struct LexerTextBuffer
{
    int32_t counter;
    uint32_t s;
    uint32_t n;
    char* buffer;
};

static __thread struct LexerTextBuffer brace_buffer = {0};

static inline void ll_add_to_brace(const char* lex_text, uint32_t len);
static inline void ll_match_brace(ParsingStack* lex_state);

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

static inline void ll_match_brace(ParsingStack* lex_state)
{
    NEOAST_STACK_PUSH(lex_state, S_MATCH_BRACE);
    brace_buffer.s = 1024;
    brace_buffer.buffer = malloc(brace_buffer.s);
    brace_buffer.n = 0;
    brace_buffer.counter = 1;
}

}

%option parser_type="LALR(1)"
%option prefix="cc"
%option track_position="TRUE"
%option debug_table="FALSE"

%union {
    char* identifier;
    char ascii;
    key_val_t key_val_type;
    struct KeyVal* key_val;
    struct LexerRuleProto* l_rule;
    struct Token* token;
    struct GrammarRuleSingleProto* g_single_rule;
    struct GrammarRuleProto* g_rule;
    struct File* file;
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
%token END_STATE
%token<key_val> MACRO
%token BOTTOM
%token TOKEN // %token
%token DESTRUCTOR
%token<ascii> ASCII
%token<identifier> LITERAL
%token<identifier> ACTION
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
%type<l_rule> lexer_rule
%type<token> tokens
%type<token> token

%type <file> program
%start <file> program

%destructor<identifier> { free($$); }
%destructor<l_rule> { lexer_rule_free($$); }
%destructor<g_single_rule> { grammar_rule_single_free($$); }
%destructor<g_rule> { grammar_rule_multi_free($$); }
%destructor<key_val> { key_val_free($$); }
%destructor<token> { tokens_free($$); }
%destructor<file> { file_free($$); }

+literal        \"(\\.|[^\"\\])*\"
+identifier     [A-Za-z_][\w]*
+lex_state      <[A-Za-z_][\w]>[\s]*'{'
+ascii          '[\x20-\x7E]'
+macro          \+{identifier}[ ][\s]*[^\n]+
+whitespace     [ \t\r]+

==

// Initial state

// Whitespace
"[\n]"              { position->line++; }
"{whitespace}"      { /* skip */ }
"//[^\n]*"          { /* skip */ }
"/\*"               { NEOAST_STACK_PUSH(lex_state, S_COMMENT); }

// These take precedence over literal tokens
"{literal}"         { yyval->identifier = strndup(yytext + 1, len - 2); return LITERAL; }
"{ascii}"           { yyval->ascii = yytext[1]; return ASCII; }

// Don't build anything during lexing to keep things simple
// This is different from the way the bootstraping compiler does it
"=="                { NEOAST_STACK_PUSH(lex_state, S_LL_RULES); return LL; }
"%%"                { NEOAST_STACK_PUSH(lex_state, S_GG_RULES); return GG; }
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
                        yyval->key_val = key_val_build(position, KEY_VAL_MACRO, key, value);
                        return MACRO;
                    }
"{identifier}"      { yyval->identifier = strdup(yytext); return IDENTIFIER; }
"{"                 { ll_match_brace(lex_state); }

<S_LL_RULES> {
"[\n]"              { position->line++; }
"{whitespace}"      { /* skip */ }
"//[^\n]*"          { /* skip */ }
"/\*"               { NEOAST_STACK_PUSH(lex_state, S_COMMENT); }
"=="                { NEOAST_STACK_POP(lex_state); return LL; }
"{lex_state}"       {
                        const char* start_ptr = strchr(yytext, '<');
                        const char* end_ptr = strchr(yytext, '>');
                        yyval->identifier = strndup(start_ptr + 1, end_ptr - start_ptr - 1);
                        NEOAST_STACK_PUSH(lex_state, S_LL_STATE);

                        return LEX_STATE;
                    }
"{literal}"         { yyval->identifier = strndup(yytext + 1, len - 2); return LITERAL; }
"{"                 { ll_match_brace(lex_state); }
}

<S_LL_STATE> {
"[\n]"              { position->line++; }
"{whitespace}"      { /* skip */ }
"//[^\n]*"          { /* skip */ }
"/\*"               { NEOAST_STACK_PUSH(lex_state, S_COMMENT); }

"{literal}"         { yyval->identifier = strndup(yytext + 1, len - 2); return LITERAL; }

"{"                 { ll_match_brace(lex_state); }
"}"                 { NEOAST_STACK_POP(lex_state); return END_STATE; }

}

<S_GG_RULES> {
"[\n]"              { position->line++; }
"{whitespace}"      { /* skip */ }
"//[^\n]*"          { /* skip */ }
"{ascii}"           { yyval->ascii = yytext[1]; return ASCII; }
"/\*"               { NEOAST_STACK_PUSH(lex_state, S_COMMENT); }
"%%"                { NEOAST_STACK_POP(lex_state); return GG; }
"{identifier}"      { yyval->identifier = strdup(yytext); return IDENTIFIER; }
":"                 { return ':'; }
"\|"                { return '|'; }
";"                 { return ';'; }
"{"                 { ll_match_brace(lex_state); }
}

<S_MATCH_BRACE> {
"[\n]"              { position->line++; ll_add_to_brace(yytext, len); }

// Add ascii charaters
"'[\x00-\x7F]'"     { ll_add_to_brace(yytext, len); }

// String literals
"\"(\\.|[^\"\\])*\"" { ll_add_to_brace(yytext, len); }

// Everything else
"([^}{\"\'\n]+)"      { ll_add_to_brace(yytext, len); }

"{"                 { brace_buffer.counter++; brace_buffer.buffer[brace_buffer.n++] = '{'; }
"}"                 {
                        brace_buffer.counter--;
                        if (brace_buffer.counter <= 0)
                        {
                            // This is the outer-most brace
                            brace_buffer.buffer[brace_buffer.n++] = 0;
                            NEOAST_STACK_POP(lex_state);
                            yyval->identifier = strndup(brace_buffer.buffer, brace_buffer.n);
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
"[\n]"              { position->line++; }
"[^\*\n]+"          { /* Absorb comment */ }
"\*/"               { NEOAST_STACK_POP(lex_state); }
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
    | DESTRUCTOR '<' IDENTIFIER '>' ACTION  { $$ = declare_destructor($p1, $3, $5); }
    | RIGHT tokens                          { $$ = declare_right($2); }
    | LEFT tokens                           { $$ = declare_left($2); }
    | TOP ACTION                            { $$ = declare_top($p1, $2); }
    | BOTTOM ACTION                         { $$ = declare_bottom($p1, $2); }
    | UNION ACTION                          { $$ = declare_union($p1, $2); }
    | MACRO                                 { $$ = $1; }
    ;

header: pair                        { $$ = $1; }
      | pair header                 { $$ = $1; $$->next = $2; }
      ;

lexer_rule: LITERAL ACTION          { $$ = declare_lexer_rule($p1, $1, $2); }
          ;

lexer_rules: lexer_rule             { $$ = $1; }
           | lexer_rule lexer_rules { $$ = $1; $$->next = $2; }
           | LEX_STATE lexer_rules END_STATE { $$ = declare_state_rule($p1, $1, $2); }
           ;

single_grammar: tokens ACTION                   { $$ = declare_single_grammar($p2, $1, $2); }
              ;

multi_grammar: single_grammar                   { $$ = $1; }
             | single_grammar '|' multi_grammar { $$ = $1; $$->next = $3; }

             // Empty rule
             | ACTION                           { $$ = declare_single_grammar($p1, NULL, $1); }
             ;

grammar: IDENTIFIER ':' multi_grammar ';'       { $$ = declare_grammar($p1, $1, $3); }
       ;


grammars: grammar                   { $$ = $1; }
        | grammar grammars          { $$ = $1; $$->next = $2; }
        ;

program: header LL lexer_rules LL GG grammars GG { $$ = declare_file($1, $3, $6); }
       ;

%%
