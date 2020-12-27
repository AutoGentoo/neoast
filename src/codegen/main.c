//
// Created by tumbar on 12/24/20.
//

#include <parser.h>
#include <util/util.h>
#include "lexer.h"
#include "codegen.h"

static
const char* test_input =
        "%option testoption=\"testvalue\"\n"
        "%{\n"
        "#include <header.h>\n"
        "#include <another_header.h>\n"
        "int some_function(void) { return 0; }\n"
        "%}\n";

int main(int argc, const char* argv[])
{
    GrammarParser parser;
    if (gen_parser_init(&parser))
        return 1;

    U32 token_table[1024];
    CodegenUnion value_table[1024];

    int tok_n = lexer_fill_table(test_input, &parser, token_table,
                     value_table, 1024, 1024);

    printf("tokens: %d\n", tok_n);

    parser_free(&parser);
    return 0;
}