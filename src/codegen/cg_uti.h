#ifndef NEOAST_CG_UTI_H
#define NEOAST_CG_UTI_H

#include <stdint.h>
#include <codegen/codegen.h>

void put_enum(int start, int n, const char* const* names, FILE* fp);
const char* check_grammar_arg_skip(const char* start, const char* search);
int get_named_token(const char* token_name, const char* const* tokens, uint32_t token_n);
int codegen_index(const char* const* array, const char* to_find, int n);
int codegen_parse_bool(const KeyVal* self);

static void codegen_handle_option(
        struct Options* self,
        const KeyVal* option);

#endif //NEOAST_CG_UTI_H
