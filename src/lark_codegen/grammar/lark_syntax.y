/*
 * This file is part of the Neoast framework
 * Copyright (c) 2021 Andrei Tumbar.
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, version 3.
 *
 * This program is distributed in the hope that it will be useful, but
 * WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the GNU
 * General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program. If not, see <http://www.gnu.org/licenses/>.
 */

%include {
    #include <lark_codegen/ast.h>
    #include <iostream>

    using namespace neoast::ast;
}

%top {

struct LexerTextBuffer
{
    int32_t counter;
    std::string buffer;
    TokenPosition start_position;
};

static struct LexerTextBuffer brace_buffer = {0};

static inline void ll_add_to_brace(const char* lex_text, uint32_t len);
static inline void ll_match_brace(const TokenPosition* start_position);

}

%bottom {

static inline void ll_add_to_brace(const char* lex_text, uint32_t len)
{
    brace_buffer.buffer += std::string(lex_text, len);
}

static inline void ll_match_brace(const TokenPosition* start_position)
{
    brace_buffer.buffer = "";
    brace_buffer.start_position = *start_position;
    brace_buffer.counter = 1;
}

}

%option parser_type="LALR(1)"
%option prefix="cc"
%option annotate_line="FALSE"

%union {
    int number;
    Ast* ast;
    Token* token;
    Tokens* tokens;
    Value* value;
    KVStmt* kv_stmt;
    Stmt* stmt;
    LexerRule* lexer_rule;
    LexerState* lexer_state;
    TokenRule* token_rule;
    Expr* expr;
    Expansion* expansion;
    OpExpr* op_expr;
    OrExpr* or_expr;
    Op op;
    Atom* atom;
    Production* production;
    ManualDeclaration* manual_declaration;
    Vector_Production* productions;
    DeclarationType declaration_type;
    Declaration* declaration;
    Vector_Production* vector_production;
    Vector_LexerRule* vector_lexer_rule;
    Vector_Ast* vector_ast;
    Grammar* grammar;
    CompilerCompiler* cc;
}

%destructor<token>          { delete $$; }
%destructor<value>          { delete $$; }
%destructor<lexer_rule>     { delete $$; }
%destructor<expr>           { delete $$; }
%destructor<atom>           { delete $$; }
%destructor<production>     { delete $$; }
%destructor<productions>    { delete $$; }
%destructor<declaration>    { delete $$; }
%destructor<manual_declaration>    { delete $$; }
%destructor<grammar>        { delete $$; }
%destructor<ast>            { delete $$; }
%destructor<cc>             { delete $$; }

// Statements
%token INCLUDE OPTION START TYPE TOP BOTTOM TOKEN MACRO IMPORT DECLARE
%token LEFT RIGHT

%token '<' '>' ':' '|' ';' '='

%token T_CLASS T_ENUM T_VECTOR
%token ELLIPSE ARROW
%token<number> NUMBER
%token '(' ')' '[' ']' '~' '?' '*' '+'

%token END_STATE

%token<token> IDENTIFIER LITERAL LEX_STATE ACTION REGEX
%type <tokens> tokens
%type <declaration_type> declaration_type
%type <declaration> declaration
%type <manual_declaration> manual_declaration

%type <kv_stmt> kv_stmt
%type <stmt> stmt
%type <value> value
%type <expr> expr _expr
%type <expansion> expansion
%type <or_expr> or_expr
%type <op_expr> op_expr
%type <expr> complex_expansion
%type <op> op
%type <atom> atom

%type <token_rule> token_rule
%type <lexer_rule> lexer_rule
%type <lexer_state> lexer_state
%type <production> production
%type <vector_production> productions
%type <vector_lexer_rule> lexer_rules
%type <vector_ast> items
%type <grammar> grammar

%type <ast> item
%type <cc> program
%start <cc> program

+literal        (?:\"(\\.|[^\"\\])*\"|\'(\\.|[^\'\\])*\')
+regex          \/(?:\\\/|[^\/])+\/
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

"="                 { return '='; }
"<"                 { return '<'; }
">"                 { return '>'; }
":"                 { return ':'; }
"\|"                { return '|'; }
";"                 { return ';'; }
"\+"                { return '+'; }
"\["                { return '['; }
"\]"                { return ']'; }
"\("                { return '('; }
"\)"                { return ')'; }
"~"                 { return '~'; }
"\?"                { return '?'; }
"\*"                { return '*'; }
"\+"                { return '+'; }

"->"                { return ARROW; }
"%import\b"         { return IMPORT; }
"%option\b"         { return OPTION; }
"%macro\b"          { return MACRO; }
"%token\b"          { return TOKEN; }
"%start\b"          { return START; }
"%type\b"           { return TYPE; }
"%left\b"           { return LEFT; }
"%right\b"          { return RIGHT; }
"%top\b"            { return TOP; }
"%include\b"        { return INCLUDE; }
"%bottom\b"         { return BOTTOM; }
"%declare\b"        { return DECLARE; }
"class\b"           { return T_CLASS; }
"enum\b"            { return T_ENUM; }
"vector\b"          { return T_VECTOR; }

"[0-9]+"            { yyval->number = std::stol(yytext); return NUMBER; }
"\.\.\."            { return ELLIPSE; }

// These take precedence over literal tokens
"{identifier}"      { yyval->token = new Token(yyposition, yytext); return IDENTIFIER; }
"{literal}"         { yyval->token = new Token(yyposition, std::string(yytext + 1, yylen - 2)); return LITERAL; }
"{regex}"           { yyval->token = new Token(yyposition, std::string(yytext + 1, yylen - 2)); return REGEX; }

"\{"                { yypush(S_MATCH_BRACE); ll_match_brace(yyposition); }

"{lex_state}"       {
                        const char* start_ptr = strchr(yytext, '<');
                        const char* end_ptr = strchr(yytext, '>');
                        yyval->token = new Token(yyposition, std::string(start_ptr + 1, end_ptr - start_ptr - 1));
                        yypush(S_LL_STATE);

                        return LEX_STATE;
                    }

<S_LL_STATE> {
"{whitespace}"      { /* skip */ }
"//[^\n]*"          { /* skip */ }
"/\*"               { yypush(S_COMMENT); }

"{literal}"         { yyval->token = new Token(yyposition, std::string(yytext + 1, yylen - 2)); return LITERAL; }

"\{"                { yypush(S_MATCH_BRACE); ll_match_brace(yyposition); }
"\}"                { yypop(); return END_STATE; }

}

<S_MATCH_BRACE> {
// TODO: Comments inside braces should not be ignored because we want codegen to paste these in
"/\*"               { yypush(S_COMMENT); }

// Line comments
"//[^\n]*"          { ll_add_to_brace(yytext, yylen); }

// ASCII literals
"{ascii}"           { ll_add_to_brace(yytext, yylen); }

// String literals
"\"(\\.|[^\"\\])*\"" { ll_add_to_brace(yytext, yylen); }

// Everything else
"[^\}\{\"\'/]+"     { ll_add_to_brace(yytext, yylen); }
"/"                 { ll_add_to_brace(yytext, yylen); }

"\{"                { brace_buffer.counter++; brace_buffer.buffer += '{'; }
"\}"                {
                        brace_buffer.counter--;
                        if (brace_buffer.counter <= 0)
                        {
                            // This is the outer-most brace
                            yypop();
                            yyval->token = new Token(yyposition, brace_buffer.buffer);
                            brace_buffer.buffer = "";
                            return ACTION;
                        }
                        else
                        {
                            brace_buffer.buffer += '}';
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

kv_stmt:
    // Key value statements
      OPTION IDENTIFIER '=' LITERAL         { $$ = new KVStmt; $$->type = KVStmt::OPTION; $$->key = up<Token>($2); $$->value = up<Ast>($4); }
    | TOKEN '<' IDENTIFIER '>' tokens       { $$ = new KVStmt; $$->type = KVStmt::TOKEN; $$->key = up<Token>($3); $$->value = up<Ast>($5); }
    | MACRO IDENTIFIER REGEX                { $$ = new KVStmt; $$->type = KVStmt::TOKEN; $$->key = up<Token>($2); $$->value = up<Ast>($3); }
    ;

stmt:
    // Single value statements
//      TOKEN tokens                  { $$ = new Stmt; $$->type = Stmt::TOKEN; $$->value = up<Token>($2); }
      START IDENTIFIER              { $$ = new Stmt; $$->type = Stmt::START; $$->value = up<Ast>($2); }
    | RIGHT tokens                  { $$ = new Stmt; $$->type = Stmt::RIGHT; $$->value = up<Ast>($2); }
    | LEFT tokens                   { $$ = new Stmt; $$->type = Stmt::LEFT; $$->value = up<Ast>($2); }
    | TOP ACTION                    { $$ = new Stmt; $$->type = Stmt::TOP; $$->value = up<Ast>($2); }
    | INCLUDE ACTION                { $$ = new Stmt; $$->type = Stmt::INCLUDE; $$->value = up<Ast>($2); }
    | BOTTOM ACTION                 { $$ = new Stmt; $$->type = Stmt::BOTTOM; $$->value = up<Ast>($2); }
    | IMPORT LITERAL                { $$ = new Stmt; $$->type = Stmt::IMPORT; $$->value = up<Ast>($2); }
    ;

manual_declaration:
    DECLARE IDENTIFIER LITERAL LITERAL
        { $$ = new ManualDeclaration; $$->name = up<Token>($2); $$->c_typename = up<Token>($3); $$->c_default = up<Token>($4); }
    | DECLARE IDENTIFIER LITERAL LITERAL LITERAL
        { $$ = new ManualDeclaration; $$->name = up<Token>($2); $$->c_typename = up<Token>($3); $$->c_default = up<Token>($4);
         $$->c_delete = up<Token>($5); }
    ;

tokens: IDENTIFIER                  { $$ = new Tokens; $$->tokens.emplace_front($1); }
      | IDENTIFIER tokens           { $$ = $2; $$->tokens.emplace_front($1); }
      ;

value: IDENTIFIER                   { $$ = new Value; $$->type = Value::TOKEN; $$->token = up<Token>($1); }
     | LITERAL                      { $$ = new Value; $$->type = Value::LITERAL; $$->token = up<Token>($1); }
     | REGEX                        { $$ = new Value; $$->type = Value::REGEX; $$->token = up<Token>($1); }
     ;

atom: value                         { $$ = new Atom; $$->value = up<Value>($1); }
    | value '<' IDENTIFIER '>'      { $$ = new Atom; $$->value = up<Value>($1); $$->name = up<Token>($3); }
    ;

_expr:
    atom
    | '(' complex_expansion ')'     { $$ = $2; }
    | '[' complex_expansion ']'     { { auto expr = new OpExpr; expr->inner = up<Expr>($2); expr->op = Op::ZERO_OR_ONE; $$ = expr; } }
    ;

op: '?'                             { $$ = Op::ZERO_OR_ONE; }
  | '*'                             { $$ = Op::ZERO_OR_MORE; }
  | '+'                             { $$ = Op::ONE_OR_MORE; }
  ;

op_expr:
    _expr op                        { $$ = new OpExpr; $$->inner = up<Expr>($1); $$->op = $2; }
//    | _expr '~' NUMBER              { $$ = new OpExpr; $$->inner = up<Expr>($1); $$->op = Op::Op_NONE; $$->n1 = $3; }
//    | _expr '~' NUMBER ELLIPSE NUMBER { $$ = new OpExpr; $$->inner = up<Expr>($1); $$->op = Op::Op_NONE; $$->n1 = $3; $$->n2 = $5; }
    ;

expr: op_expr | _expr ;

expansion: expr                     { $$ = new Expansion; $$->all.emplace_front($1); }
         | expr expansion           { $$ = $2; $$->all.emplace_front($1); }
         ;

or_expr:
    expr '|' expr                   { $$ = new OrExpr; $$->left = up<Expr>($1); $$->right = up<Expr>($3); }
    ;

complex_expansion:
          expansion
        | or_expr
        ;

production: expansion                    { $$ = new Production; $$->rule.splice($$->rule.begin(), $1->all); delete $1; }
          | expansion ARROW IDENTIFIER   { $$ = new Production; $$->rule.splice($$->rule.begin(), $1->all); delete $1; $$->alias = up<Token>($3); }
          | expansion ACTION             { $$ = new Production; $$->rule.splice($$->rule.begin(), $1->all); delete $1; $$->action = up<Token>($2); }
          | ACTION                       { $$ = new Production; $$->action = up<Token>($1); }
          ;

productions: production                 { $$ = new Vector_Production; $$->all.emplace_front($1); }
           | production '|' productions { $$ = $3; $$->all.emplace_front($1); }
           ;

declaration_type:
              T_CLASS               { $$ = DeclarationType::CLASS; }
            | T_ENUM                { $$ = DeclarationType::ENUM; }
            | T_VECTOR              { $$ = DeclarationType::VECTOR; }
            ;


declaration:
              declaration_type IDENTIFIER
                                    { $$ = new Declaration; $$->type = $1; $$->name = up<Token>($2); }
            | declaration_type IDENTIFIER ':' IDENTIFIER
                                    { $$ = new Declaration; $$->type = $1; $$->name = up<Token>($2); $$->extends = up<Token>($4); }
            | declaration_type IDENTIFIER ACTION
                                    { $$ = new Declaration; $$->type = $1; $$->name = up<Token>($2); $$->extra_code = up<Token>($3); }
            | declaration_type IDENTIFIER ':' IDENTIFIER ACTION
                                    { $$ = new Declaration; $$->type = $1; $$->name = up<Token>($2); $$->extends = up<Token>($4); $$->extra_code = up<Token>($5); }
            ;

grammar: '<' declaration '>' IDENTIFIER ':' productions ';'
                                    { $$ = new Grammar; $$->generates = up<Declaration>($2); $$->name = up<Token>($4); $$->productions.splice($$->productions.begin(), $6->all); delete $6; }
       | '<' IDENTIFIER '>' IDENTIFIER ':' productions ';'
                                    { $$ = new Grammar; $$->type = up<Token>($2); $$->name = up<Token>($4); $$->productions.splice($$->productions.begin(), $6->all); delete $6; }
       ;

token_rule: '+' IDENTIFIER ':' REGEX { $$ = new TokenRule; $$->name = up<Token>($2); $$->regex = up<Token>($4); }
          ;

lexer_rule: REGEX ACTION            { $$ = new LexerRule; $$->regex = up<Token>($1); $$->action = up<Token>($2); }
          ;

lexer_rules: lexer_rule             { $$ = new Vector_LexerRule; $$->all.emplace_front($1); }
           | lexer_rule lexer_rules { $$ = $2; $$->all.emplace_front($1); }
           ;

lexer_state: '<' IDENTIFIER '>' '[' lexer_rules ']'
                                    {
                                        $$ = new LexerState; $$->name = up<Token>($2);
                                        $$->rules.splice($$->rules.begin(), $5->all);
                                        delete $5;
                                    }
           ;

item: kv_stmt
    | manual_declaration
    | stmt
    | lexer_rule
    | token_rule
    | lexer_state
    | grammar
    ;

items: item                         { $$ = new Vector_Ast; $$->all.emplace_front($1); }
     | item items                   { $$ = $2; $$->all.emplace_front($1); }
     ;

program: items                      {
                                        $$ = new CompilerCompiler;
                                        $$->items.splice($$->items.begin(), $1->all);
                                        delete $1;
                                    }
       |                            { $$ = new CompilerCompiler; }
       ;

%%
