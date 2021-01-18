# neoast
A modern AST grammar lexer/parser generator written in C. Neoast can take arbitrary
lexing and grammar rules and generate a scanner and parsing table. Unlike Bison/Flex,
neoast does not generate the scanner and parsing runtime for each input, it uses the
same algorithm and links a static runtime library. This means that many optimizations
can be made when comparing to Bison.


## Usage
Neoast is shipped with a `CMakeLists.txt` script that can build all of the dependencies
to the lexing and parsing runtime. If you already have a CMake project, you can add this
project as a sub-project:

```cmake
add_subdirectory(neoast EXCLUDE_FROM_ALL)
```

Now you will have access to a macro to build a parser given an input file:
```cmake
BuildParser(parser_name path/to/my/parser.y)

# Add the parser source code to an executable
add_executable(exec
        main.c ${parser_name_OUTPUT})

# Link the runtime library
target_link_libraries(exec PRIVATE neoast)
```

## Examples
Check `test/input/` for example input files.

## What is a Lexer and Parser
A lexer is a program that will break up an input into tokens. For example if we have a simple
C expression:
```C
fprintf(stderr, "Hello world %s\n", "===");
```

A lexer may break up this expression into something like this:
```
IDENTIFIER '(' IDENTIFIER ',' STRING_CONSTANT ',' STRING_CONSTANT ')' ';'
```

This is whats known as a token table. Each value in the token was matched by a regular
expression and built using a lexing rule. Notice that the value corresponding to each
token is not indicated in the token table. Like tokens, values are stored in a separate
table known as the value table. The value table is an array of `%union` objects. See the
`%union` section for more information.

After an input string/buffer has been tokenized with the lexer, its time for the parser
to reduce sequences of tokens into expressions. This is done through grammar rules.

Neoast can generate `LALR(1)` and `CLR(1)` parsing tables and using the `LR` parsing
algorithm to reduce grammar rules. If you don't know what this is its not super important.
By default, neoast will generate a `LALR(1)` parser but this can be changed via the `parser_type`
`%option`.

## Lexer/Parser files
Neoast can read input files and generate a C source file based on the
the contents. This source file will initialize a parser with compile-time
constants such as a LR parsing table and lexing rules.

There are three distinct sections to a parser input file:
```
[HEADER]

==
[LEXING RULE]
==

%%
[grammar RULES]
%%
```

### Header section
The header section is where all of the metadata and declarations occur.
Here you will define the value `%union` and as well as `%token`s and `%type`s.
The header section also takes `%option`s that control different features
of neoast code generation. Below we discuss each header switch that is parsed
by neoast

#### `%top` and `%bottom`
`%top` is where we define C source code that will be placed at
the top of the generated C file. Usually header `#include`s
will go here. For example:

```C
%top {
    #include <stdio.h> // printf()
    #include <math.h>  // pow()
}
```

Similarly `%bottom` is source code that will be placed at the bottom of the
source file. Here you'll probably place extra functions that depend on functions
that are generated during code generation and therefore need to be placed after they
are declared.

#### `%union`
`%union` is the most important switch in the input file. It defines that data type that
will store the Abstract-Syntax-Tree (AST) from the parser. During lexing and parsing, the
programmer will have access to objects with this union type. The different fields in the
`union` are used to provide type to `%token`s and `%type`.

```C
%union {
    char* identifier;
    struct {
        char* key;
        char* value;
    } key_val;
}
```

#### `%token` and `%type`
`%token` and `%type` define the lexing and parsing tokens respectively.
`%token` is used for anything found during lexing including ASCII characters.
`%type` is used to define an expression that the parser can reduce to.
More on this later.

Here as example of the valid syntax to use with these switches
```C
// Token declarations
%token MY_TOKEN_NAME // A simple token
%token ';'  // ascii token (no unicode allowed, define a simple token for this)
%token <my_union_field> MY_TYPED_TOKEN  // typed token

// Type declarations
%type <my_union_field> MY_TYPE
```

Notice the special syntax to select the type `<field_name>` of a `%token` or `%type`.
A `%type` must include a type where a `%token` does not necessarily need one. The field
name must be a valid field name in the `%union`. When the token/type is used in a grammar
rule, their union field will be used. This allows the programmer to easily declare different
expressions and tokens with types and for the runtime library to not have to worry about
these types during parsing.

#### `%destructor`
A destructor will tell the parser how to free a union field if it runs into a syntax error.
For example, if you allocate a string during lexing and place the pointer in the value table,
you may want to free this memory if the parser runs into an error. Here's an example:

```C
%union {
    char* identifier;
    struct {
        char* key;
        char* value;
    } key_val;
};

%destructor <identifier> {free($$);}
%destructor <key_val> {free($$.key); free($$.value);}
```

> Note: `$$` denotes the value in the value table

#### `%left` and `%right`
These switches are used to resolve **SR** conflicts. See below.

#### Regex macros
Regex macros are used to simplify a programmers life when defining lexing rules. They
are similar to C macros as they expand at compile time. Here's an example:

```
+letter            A-Za-z
+digit             0-9
+identifier        [{letter}_][{letter}{digit}_]*
```

Notice how macros don't necessarily need to be valid regex however once the macro is
fully expanded into a regular expression for a lexer rule, it must be valid regex

### Lexer section
In this section you will define the regular expressions used to match different tokens.
Neoast used Google's RE2 as a regular expression engine. This means its limited to these
rules and will fail to compile if you attempt to use an invalid regular expression. Here
is an example of a lexer rule:

```C
"[A-Z_a-z][A-Za-z_0-9]*"             {yyval->identifier = strdup(yytext); return IDENTIFIER;}

// Or we can use the macro defined above
"{identifier}"                       {yyval->identifier = strdup(yytext); return IDENTIFIER;}
```

> Note: All regular expressions in the lexer sections must be inside a C-string with double quotes.

This is a common declaration when trying to match an identifier in most languages. Here we saying
that every identifier must have a letter or underscore followed by any combination of letters,
underscores and digits to declare an identifier. The bracketed statement to the right of the regular
expression is known as an action. A lexer actions has a couple of valid local variables associated 
with it. Here's how a lexer action is defined:

```C
static int32_t
ll_rule_LEX_STATE_DEFAULT_01(const char* yytext, NeoastValue* yyval,
                             uint32_t len, ParsingStack* lex_state)
{
    // These are purely used to ignore compiler warnings for
    // 'unused variables'
    (void) yytext;
    (void) yyval;
    (void) len;
    (void) lex_state;

    {yyval->identifier = strdup(yytext); return IDENTIFIER;}
    return -1;
}
```

| Local Name | Purpose |
| ---------- | ------- |
| `yytext`   | Raw text matched by the regular expression (constant). |
| `yyval`    | A pointer to the value in the value table. `NeoastValue` is the `union` generated by `%union` |
| `len`      | Same as `strlen(yytext)`. Use this for optimizations because we already know this strings length. |
| `lex_state`| We can `push` are or `pop` from this stack to go to different lexing states. See Lexing states for more details. |

Notice that if the defined action does not return, the function returns `-1`.
`-1` is a special token used to tell the lexer to skip this block of text. You'll most likely use this
when defining rules for `[ \n\t]+` or whitespace.

### Grammar section
The grammar section will define the grammar rules used to reduce token
sequences into expressions. The same syntax used in bison is utilized in neoast.

A grammar rule is just a sequence of tokens followed by an action:
```C
%union {
    char* str;
    struct Expression {
        ...
        struct Expression* next;
    }* expression;
}

// Every grammar must have a corresponding %type declaration
%type <expression> function_call
%type <expression> argument_list
%type <expression> expression
%token <string> IDENTIFIER
%token '('
%token ')'
%token ','

%%
expression      : function_call                {$$ = $1;}
                | IDENTIFIER                   {$$ = build_identifier($1);}
                ;

function_call   :  expression '(' argument_list ')'  {$$ = build_function_call($1, $3);}
                ;

argument_list   : expression ',' argument_list       {$$ = $1; $$->next = $3;}
                | expression                         {$$ = $1; $$->next = NULL;}
                ;

%%
```

This grammar will define a very simple language where all you can do is call
functions with arguments. Notice that the `$[0-9]+` variables are available
in grammar actions to get the value of a token from the value table. ONLY tokens
with a defined type can be used here, if you tried to use a token without a type defined,
you'll get a compilation error.

#### SR and RR conflicts
The parsing table is an array used to define a finite state machine to
keep track of parsing states will the parser is making sense of an input.
There are three types of operations embedded in the parsing table, `SHIFT`,
`REDUCE` and `GOTO`.

Shift is used when a token is seen by the parser which the
parser expects. When this token is seen, the parser will push this token to the stack
along with the state its in (state in the finite stack machine).

A reduce operation happens when the parser sees the entire token
sequence of a grammar rule is seen by the parser and its ready to
reduce this sequence to the `%type` token (`function_call` for example).

After a reduce operation happens, it needs to know which state to proceed
to which is done via the Goto operation.

Ok, know that those concepts are explained, on to conflicts. A conflict occurs
when grammar rule is ambiguous and could be interpreted in multiple ways.

An SR conflict happens when a shift or reduce operation is valid for the same
location in the parsing table. If that means nothing to you take the example below:

```
// Simple addition grammar
expression : NUMBER
           | expression '+' expression
           ;
```

and the following input:
```
5 + 6 + 7
```

Now think of how a parser would reduce this:
```
// Option 1   (reduce)
(5 + 6) + 7

// Option 2   (shift)
5 + (6 + 7)
```

To solve these type of issues, neoast has `%left` and `%right`
switches to determine operation precedence for ambiguous grammar rules.

```
// Reduce the expression before continuing
%left '+'

// OR
// We want option 2 instead of 1
%right '+'
```

The second type of conflict, RR conflict, is much more rare than the
SR conflict. It happens when two rules can validly be reduced at the same
time. This is simply an error in the grammar and cannot be fixed via a
header switch.

## Contributing
If you want to contribute or submit a bug report/feature request, you are
always welcome to do so.

Any contribution should be make via a pull request. When you open a pull
request please request a review from @kronos3.

I will not merge any PR that does not pass all of the CI tests (including
the sanitizer tests (ASAN + UBSAN)).

If you have any questions, you can always open an issue and mark it with
the "question" tag.