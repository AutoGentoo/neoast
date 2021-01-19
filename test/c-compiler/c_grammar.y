%top {
    // Yay a C compiler!!
    // This C compiler is implemented like Clang that
    // compile to LLVM source.
    // This way we compile to any target architecture

    #include <stdint.h>
}

%union {
    char* id;
    uint64_t integer;
    double floating;
}

%token '{'

%token<id> ID
%token T_IF

+letter      a-zA-Z
+digit       0-9
+letdig      {letter}{digit}
+id          [{letter}_][{letdig}_]*

==

"[ \t\n]+"   {} // skip whitespace
"//[^\n]+"   {} // skip line comments

"{id}"              {llval->id = strdup(yytext); return ID;}
"0x[{digit}]+"      {llval->integer = strtoul(yytext + 2, NULL, 16); return NUMBER;}
"[-]?[{digit}]+"    {llval->integer = strtol(yytext, NULL, 10); return NUMBER;}

// We want keywords to come last just in case we have an identifier
// with a keyword embedded in its name

// Branching keywords
"if"         {return T_IF;}
"else"       {return T_ELSE;}
"while"      {return T_WHILE;}
"for"        {return T_FOR;}
"continue"   {return T_CONTINUE;}
"break"      {return T_BREAK;}
"switch"     {return T_SWITCH;}
"case"       {return T_CASE;}
"default"    {return T_DEFAULT;}
"do"         {return T_DO;}
"goto"       {return T_DO;}
"return"     {return T_RETURN;}

// Qualifiers
"signed"     {return T_SIGNED;}
"unsigned"   {return T_UNSIGNED;}
"register"   {return T_REGISTER;}
"const"      {return T_CONST;}
"static"     {return T_STATIC;}
"extern"     {return T_EXTERN;}
"inline"     {return T_INLINE;}
"volatile"   {return T_VOLATILE;}
"__thread"   {return T_THREAD;}

// Built-in types
"void"       {return T_VOID;}
"char"       {return T_CHAR;}
"short"      {return T_SHORT;}
"int"        {return T_INT;}
"long"       {return T_LONG;}
"float"      {return T_FLOAT;}
"double"     {return T_DOUBLE;}

// Specials
"enum"       {return T_ENUM;}
"typedef"    {return T_TYPEDEF;}
"struct"     {return T_STRUCT;}
"union"      {return T_UNION;}
"sizeof"     {return T_SIZEOF;}

// Ascii character definitions
"{"          {return '{';}
"}"          {return '}';}
"("          {return '(';}
")"          {return ')';}
"+"          {return '+';}
"-"          {return '-';}
"="          {return '=';}
"["          {return '[';}
"\\"         {return '\\';}
"|"          {return '|';}
"!"          {return '!';}
"%"          {return '%';}
"^"          {return '^';}
"&"          {return '&';}
"*"          {return '*';}
"/"          {return '/';}
"?"          {return '?';}
":"          {return ':';}
";"          {return ';';}
","          {return ',';}
"."          {return '.';}
">"          {return '>';}
"<"          {return '<';}
"'"          {return '\'';}
"\""         {return '"';}
"~"          {return '~';}

==

%%

%%