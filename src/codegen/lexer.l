%top{
  #include <cstdlib>
  #include <string>
  #include <map>

  // fixes problem with Bison 2.7 that needs the Lexer class type:
  class REFLEX_OPTION_lexer;  // = %option lexer (which is 'Lexer' by default)

  #include "bootstrap/grammar.h"
}

%{
  extern void yyerror(Lexer *lexer, const char *msg);
%}

%class{
 public:
  std::map<std::wstring,double> map;
 private:
  std::wstring                  var;
}

%option fast header-file freespace

var    \p{UnicodeIdentifierStart} \p{UnicodeIdentifierPart}*
exp    [Ee] [-+]? \d+
num    \d* (\d | \.\d | \d\.) \d* {exp}?

%%

{var}        { var = wstr(); yylval.var = &var; return 'V'; }
{num}        { yylval.num = strtod(text(), NULL); return 'N'; }
[-+*/=()]    { return *text(); }
\n           { return '\n'; }
\s           // ignore space
.            { yyerror(this, "mystery character"); }

%%
