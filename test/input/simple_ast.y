%include {
#include <stdlib.h>

typedef enum {
    USE_OP_DISABLE, //!< !
    USE_OP_ENABLE, //!< ?
    USE_OP_LEAST_ONE, //!< ||
    USE_OP_EXACT_ONE, //!< ^^
    USE_OP_MOST_ONE, //!< ??
} use_operator_t;

typedef enum {
    ATOM_DEFAULT_NONE, //!< use
    ATOM_DEFAULT_ON, //!< use(+)
    ATOM_DEFAULT_OFF, //!< use(-)
} atom_use_default_t;
typedef struct RequiredUse_prv RequiredUse;
}

%top {
struct RequiredUse_prv {
    char* target;
    use_operator_t operator;
    RequiredUse* depend;
    RequiredUse* next;
};

void required_use_stmt_free(RequiredUse* self)
{
    if (!self) return;

    required_use_stmt_free(self->depend);
    required_use_stmt_free(self->next);

    free(self->target);
    free(self);
}
}

// This is default, just want to test the parser
%option parser_type="LALR(1)"
//%option disable_locks="TRUE"
%option debug_table="TRUE"
%option annotate_line="FALSE"
%option prefix="required_use"
%option debug_ids="$ids!?()ESDRP"

%union {
    char* identifier;
    atom_use_default_t use_default;
    RequiredUse* required_use;
    struct {
        char* target;
        use_operator_t operator;
    } use_select;
}

%token <identifier> IDENTIFIER
%token <use_default> USE_DEFAULT
%token <use_select> USESELECT

%type <required_use> required_use_expr
%type <required_use> required_use_single
%type <use_select> depend_expr_sel
%type <use_select> use_expr

%destructor <identifier> { free($$); }
%destructor <use_select> { if($$.target) free($$.target); }
%destructor <required_use> { required_use_stmt_free($$); }

%token '!'
%token '?'
%token '('
%token ')'

%start<required_use> program_required_use
%type <required_use> program_required_use


+identifier      [A-Za-z_0-9][A-Za-z_0-9\-\+]*

==

"[ \t\r\\\n]+"          {/* skip */}
"\?\?"                  {yyval->use_select.target = NULL; yyval->use_select.operator = USE_OP_MOST_ONE; return USESELECT;}
"\|\|"                  {yyval->use_select.target = NULL; yyval->use_select.operator = USE_OP_LEAST_ONE; return USESELECT;}
"\^\^"                  {yyval->use_select.target = NULL; yyval->use_select.operator = USE_OP_EXACT_ONE; return USESELECT;}
"\(\+\)"                {yyval->use_default = ATOM_DEFAULT_ON; return USE_DEFAULT;}
"\(\-\)"                {yyval->use_default = ATOM_DEFAULT_OFF; return USE_DEFAULT;}
"!"                     {return '!';}
"\?"                    {return '?';}
"\("                    {return '(';}
"\)"                    {return ')';}
"{identifier}"          {
                            yyval->identifier = strdup(yytext);
                            return IDENTIFIER;
                        }

==

%%

program_required_use: required_use_expr   {$$ = $1;}
            |                             {$$ = NULL;}
            ;

required_use_expr   : required_use_single                         {$$ = $1;}
                    | required_use_single required_use_expr       {$$ = $1; $$->next = $2;}
                    ;

required_use_single : use_expr                                      {
                                                                        $$ = malloc(sizeof(RequiredUse));
                                                                        $$->target = $1.target;
                                                                        $$->operator = $1.operator;
                                                                        $$->depend = NULL;
                                                                        $$->next = NULL;
                                                                    }
                    | depend_expr_sel '(' required_use_expr ')'     {
                                                                        $$ = malloc(sizeof(RequiredUse));
                                                                        $$->target = $1.target;
                                                                        $$->operator = $1.operator;
                                                                        $$->depend = $3;
                                                                        $$->next = NULL;
                                                                    }
                    | '(' required_use_expr ')'                   {$$ = $2;}
                    ;

depend_expr_sel : use_expr '?'       {$$ = $1;}
                | USESELECT          {$$ = $1;}
                ;

use_expr     : '!' IDENTIFIER        {$$.target = $2; $$.operator = USE_OP_DISABLE;}
             | IDENTIFIER            {$$.target = $1; $$.operator = USE_OP_ENABLE;}
             ;

%%