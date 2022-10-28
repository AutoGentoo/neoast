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

// Skip whitespace and comments
/[ \t\r\n]+/        { /* skip */ }
/\/\/[^\n]*/        { /* skip */ }
/\/\*/              { yypush(S_COMMENT); }
/\{/                { yypush(S_MATCH_BRACE); ll_match_brace(yyposition); }

<S_COMMENT> [
    /[^\*]+/            { /* Absorb comment */ }
    /\*\//              { yypop(); }
    /\*/                { }
]

<S_MATCH_BRACE> [
// TODO: Comments inside braces should not be ignored because we want codegen to paste these in
/\/\*/              { yypush(S_COMMENT); }

// Line comments
/\/\/[^\n]*/        { ll_add_to_brace(yytext, yylen); }

// ASCII literals
/'([\x20-\x7E]|\\.)'/  { ll_add_to_brace(yytext, yylen); }

// String literals
/\"(\\.|[^\"\\])*\"/ { ll_add_to_brace(yytext, yylen); }

// Everything else
/[^\}\{\"\'\/]+/      { ll_add_to_brace(yytext, yylen); }
/\//                 { ll_add_to_brace(yytext, yylen); }

/\{/                { brace_buffer.counter++; brace_buffer.buffer += '{'; }
/\}/                {
                        brace_buffer.counter--;
                        if (brace_buffer.counter <= 0)
                        {
                            // This is the outer-most brace
                            yypop();
                            yyval->token = new Token(*yyposition, brace_buffer.buffer);
                            brace_buffer.buffer = "";
                            return ACTION;
                        }
                        else
                        {
                            brace_buffer.buffer += '}';
                        }
                    }
]

