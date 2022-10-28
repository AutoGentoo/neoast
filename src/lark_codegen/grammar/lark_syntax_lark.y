%option namespace="neoast::ast"
%option ast_class="Ast"
%option token_class="Token"
%option parser_type="LALR(1)"

// Defines ACTION (match brace), handles comments
%import "/home/tumbar/git/neoast/src/lark_codegen/grammar/lark_helper.y"

%token<Token> ACTION
%declare Integer "int" "-1"
%token<Integer> NUMBER

+IDENTIFIER: /[A-Za-z_][\w]*/
+REGEX:      /\/(?:\\\/|[^\/])+\//
+LITERAL:    /(?:\"(\\.|[^\"\\])*\"|\'(\\.|[^\'\\])*\')/

/[0-9]+/    { yyval->NUMBER = std::stol(yytext); return NUMBER; }

<class Tokens> tokens: IDENTIFIER<tokens>+ ;

<class KVStmt> kv_stmt:
      "%option" IDENTIFIER<key> '=' LITERAL<value>          -> OPTION
    | "%token" ['<' IDENTIFIER<key> '>'] tokens<value>      -> TOKEN
    | "%destructor" '<' IDENTIFIER<key> '>' ACTION<value>   -> DESTRUCTOR
    | "%macro" IDENTIFIER<key> REGEX<value>                 -> MACRO
    ;

<class Stmt> stmt:
      "%start" IDENTIFIER<value>                -> START
    | "%right" tokens<value>                    -> RIGHT
    | "%left" tokens<value>                     -> LEFT
    | "%top" ACTION<value>                      -> TOP
    | "%include" ACTION<value>                  -> INCLUDE
    | "%bottom" ACTION<value>                   -> BOTTOM
    | "%union" ACTION<value>                    -> UNION
    | "%import" LITERAL<value>                  -> IMPORT
    ;

<class Value> value:
      IDENTIFIER<token>    -> TOKEN
    | LITERAL<token>       -> LITERAL
    | REGEX<token>         -> REGEX
    ;

<class Atom> atom:
      value
    | value '<' IDENTIFIER<name> '>'
    ;

<Ast> _expr:
      atom<inner>
    | '(' complex_expansion ')'         { $$ = $2; }
    ;

<enum Op> op: '?' -> ZERO_OR_ONE
            | '*' -> ZERO_OR_MORE
            | '+' -> ONE_OR_MORE
            ;

<class Expr> expr:
    _expr<inner> [op | (NUMBER<n1> ['..' NUMBER<n2>])]
    | '[' complex_expansion ']'         { $$ = $2; $$->op = ZERO_OR_ONE; }
    ;

<vector Expr> expansion:
      expr<front>
    | expr<front> expansion<this>
    ;

<class OrExpr> or_expr:
    expr<left> '|' expr<right>
    ;

<Ast> complex_expansion:
      expansion<this>
    | or_expr<this>
    ;

<class Production> production:
    expansion<rule>? [ ( '->' IDENTIFIER<alias> ) | ACTION<action> ]
    ;

<vector Production> productions:
    production<front>
    | production<front> '|' productions<this>
    ;

<enum AstDeclarationType> ast_declaration_type:
      "class" -> CLASS
    | "enum" -> ENUM
    | "vector" -> VECTOR
    ;

<class AstDeclaration> ast_declaration:
    ast_declaration_type<type> IDENTIFIER<name> [':' IDENTIFIER<extends> ] ACTION<extra_code>?
    ;

<class Grammar> grammar:
    '<' ( ast_declaration<generates> | IDENTIFIER<type> ) '>'
    IDENTIFIER<name> ':' productions ';'
    ;

<class TokenRule> token_rule:
    '+' IDENTIFIER<name> ':' REGEX<regex>
    ;

<class LexerRule> lexer_rule:
    REGEX<regex> ACTION<action>
    ;

<class LexerState> lexer_state:
    '<' IDENTIFIER<name> '>' '[' lexer_rule<rules>+ ']'
    ;

<Ast> item: kv_stmt
          | stmt
          | grammar
          | token_rule
          | lexer_rule
          | lexer_state
          ;

<Ast> items: item
           | item items
           ;

%start cc_file
<class CompilerCompilerFile> cc_file:
        items
    ;

//    item<items>+
//    | { $$ = CompilerCompilerFile; }
