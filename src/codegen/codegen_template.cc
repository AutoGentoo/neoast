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


#include <codegen/codegen_impl.h>
#include <inja/inja.hpp>

static const char license_header[] = R"(/*
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
 *
 * THIS FILE HAS BEEN AUTOMATICALLY GENERATED, DO NOT CHANGE
 */

)";

void CodeGenImpl::write_header(std::ostream &os, bool dump_license) const
{
    std::ostringstream os_tokens;
    std::ostringstream os_lexer;
    put_enum(NEOAST_ASCII_MAX, tokens, os_tokens, 1);

    inja::json header_data;
    header_data["prefix"] = options.prefix;
    header_data["tokens"] = os_tokens.str();
    header_data["lexer"] = os_lexer.str();
    header_data["union"] = union_->get_simple(options);
    header_data["include"] = include_ ? include_->get_simple(options) : "";
    header_data["union_name"] = CODEGEN_UNION;
    header_data["struct_name"] = CODEGEN_STRUCT;
    header_data["start_type"] = start_type;

    if (dump_license)
    {
        os << license_header;
    }

    os << inja::render(
            R"(#ifndef __NEOAST_{{ upper(prefix) }}_H__
#define __NEOAST_{{ upper(prefix) }}_H__

{{ include }}

#ifdef __cplusplus
extern "C" {
#endif

/*************************** NEOAST Token definition ****************************/
#ifdef NEOAST_GET_TOKENS
{{ tokens }}
#endif // NEOAST_GET_TOKENS

#include <lexer/input.h>

#ifdef NEOAST_GET_STRUCTURE
/************************ NEOAST Union/Struct definition ************************/
typedef union { {{ union }} } {{ union_name }};

typedef struct {
    {{ union_name }} value;
    TokenPosition position;
} {{ struct_name }};
#endif // NEOAST_GET_STRUCTURE

/*************************** NEOAST Lexer definition ****************************/
{{ lexer }}

void* {{ prefix }}_allocate_buffers();

void {{ prefix }}_free_buffers(void* self);

extern {{ union_name }} __{{ prefix }}__t_;

/**
 * Basic entrypoint into the {{ prefix }} parser
 * @param buffers_ Allocated pointer to buffers created with {{ prefix }}_allocate_buffers()
 * @param input pointer to raw input
 * @param input_len length of input in bytes
 * @return top of the generated AST
 */
typeof(__{{ prefix }}__t_.{{ start_type }}) {{ prefix }}_parse_len(void* error_ctx, void* buffers_, const char* input, uint32_t input_len);
typeof(__{{ prefix }}__t_.{{ start_type }}) {{ prefix }}_parse(void* error_ctx, void* buffers_, const char* input);

/**
 * Advanced entrypoint into the {{ prefix }} parser
 * @param buffers_ Allocated pointer to buffers created with {{ prefix }}_allocate_buffers()
 * @param input initialized neoast input from <lexer/input.h>
 * @return top of the generated AST
 */
typeof(__{{ prefix }}__t_.{{ start_type }}) {{ prefix }}_parse_input(void* error_ctx, void* buffers_, NeoastInput* input);

#ifdef __cplusplus
}
#endif

#endif // __NEOAST_{{ upper(prefix) }}_H__
)", header_data);
}

void CodeGenImpl::write_source(std::ostream &os, bool dump_license) const
{
    // Act as a preprocessor and put the external header
    std::ostringstream os_header_preprocessor;
    write_header(os_header_preprocessor, false);

    std::ostringstream os_lexer_top;
    lexer->put_top(os_lexer_top);

    std::ostringstream os_lexer_bottom;
    lexer->put_bottom(os_lexer_bottom);

    std::ostringstream os_lexer;
    lexer->put_global(os_lexer);

    std::ostringstream os_ascii_mappings;
    put_ascii_mappings(os_ascii_mappings);

    std::ostringstream os_parsing_table;
    put_parsing_table(os_parsing_table);

    std::ostringstream os_grammar;
    grammar->put_actions(os_grammar);
    grammar->put_table(os_grammar);
    grammar->put_rules(os_grammar);

    // Convert the destructors to JSON
    inja::json destructors_json = inja::json::object();
    for (const auto &iter : destructors)
    {
        destructors_json[iter.first] = iter.second->get_complex(options, {iter.first}, "self", "", true);
    }

    inja::json destructor_table_json = inja::json::array();
    for (const auto &token_name : tokens)
    {
        auto tok = get_token(token_name);
        inja::json table_entry;
        table_entry["token_name"] = token_name;

        if (!tok) throw Exception("Failed to find token " + token_name);
        else if (!dynamic_cast<CGTyped*>(tok.get()) ||
                 destructors.find(dynamic_cast<CGTyped*>(tok.get())->type)
                 == destructors.end()) table_entry["function"] = "NULL";
        else
            table_entry["function"] = "(parser_destructor) " + dynamic_cast<CGTyped*>(tok.get())->type + "_destructor";

        destructor_table_json.push_back(std::move(table_entry));
    }

    inja::json source_data;

    /* Header defined */
    source_data["include_header"] = os_header_preprocessor.str();
    source_data["top"] = top ? top->get_simple(options) : "";
    source_data["bottom"] = bottom ? bottom->get_simple(options) : "";
    source_data["union"] = union_->get_simple(options);
    source_data["union_name"] = CODEGEN_UNION;
    source_data["struct_name"] = CODEGEN_STRUCT;
    source_data["start_type"] = start_type;

    /* Destructors */
    source_data["destructors"] = destructors_json;
    source_data["destructor_table"] = destructor_table_json;

    /* Lexer parameters */
    source_data["lexer"] = os_lexer.str();
    source_data["lexer_top"] = os_lexer_top.str();
    source_data["lexer_bottom"] = os_lexer_bottom.str();
    source_data["lexer_init"] = lexer->get_init();
    source_data["lexer_delete"] = lexer->get_delete();
    source_data["lexer_new_inst"] = lexer->get_new_inst("ll_inst");
    source_data["lexer_del_inst"] = lexer->get_del_inst("ll_inst");
    source_data["lexer_next"] = lexer->get_ll_next("ll_inst");

    /* Grammar parameters */
    source_data["grammar"] = os_grammar.str();
    source_data["grammar_n"] = grammar->size();
    source_data["action_n"] = action_tokens.size();
    source_data["parser_error"] = !options.syntax_error_cb.empty() ? options.syntax_error_cb.c_str() : "NULL";

    /* Options */
    source_data["prefix"] = options.prefix;
    source_data["max_tokens"] = options.max_tokens;
    source_data["parsing_stack_n"] = options.parsing_stack_n;

    /* Generated */

    source_data["tokens"] = tokens_names;
    source_data["ascii_mappings"] = os_ascii_mappings.str();
    source_data["parsing_table"] = os_parsing_table.str();

    if (dump_license)
    {
        os << license_header;
    }

    os << inja::render(R"(#include <neoast.h>
#include <string.h>

#define NEOAST_GET_TOKENS
#define NEOAST_GET_STRUCTURE
{{ include_header }}

{{ lexer_top }}

/************************************ TOP ***************************************/
{{ top }}

/********************************* DESTRUCTORS **********************************/
{% for key, code in destructors %}
static void
{{ key }}_destructor({{ union_name }}* self) { {{- code -}} }
{% endfor %}

// Destructor table
static const
parser_destructor __neoast_token_destructors[] = {
{%- for entry in destructor_table %}    {{ entry.function }}, // {{ entry.token_name }}
{% endfor %}
};

/************************************ LEXER *************************************/
{{ lexer }}

/*********************************** GRAMMAR ************************************/
{{ grammar }}

/******************************* ASCII MAPPINGS *********************************/
static const
uint32_t __neoast_ascii_mappings[NEOAST_ASCII_MAX] = {
{{ ascii_mappings }}
};

/***************************** NEOAST DEFINITIONS ********************************/
static const
char* __neoast_token_names[] = {
{%- for name in tokens %}        "{{ name }}",
{%- endfor -%}
};

{{ lexer_bottom }}

static GrammarParser parser = {
        .ascii_mappings = __neoast_ascii_mappings,
        .grammar_rules = __neoast_grammar_rules,
        .token_names = __neoast_token_names,
        .destructors = __neoast_token_destructors,
        .parser_error = {{ parser_error }},
        .parser_reduce = (parser_reduce) __neoast_reduce_handler,
        .grammar_n = {{ grammar_n }},
        .token_n = TOK_AUGMENT - NEOAST_ASCII_MAX,
        .action_token_n = {{ action_n }}
};

/********************************* PARSING TABLE *********************************/
{{ parsing_table }}

/******************************* PARSER DEFINITIONS ******************************/
static int parser_initialized = 0;
uint32_t {{ prefix }}_init()
{
    if (!parser_initialized)
    {
        {{ lexer_init }}
        parser_initialized = 1;
    }

    return 0;
}

void {{ prefix }}_free()
{
    if (parser_initialized)
    {
        {{ lexer_delete }}
        parser_initialized = 0;
    }
}

void* {{ prefix }}_allocate_buffers()
{
    return parser_allocate_buffers(
            {{ max_tokens }},
            {{ parsing_stack_n }},
            sizeof({{ struct_name }}),
            offsetof({{ struct_name }}, position)
    );
}

void {{ prefix }}_free_buffers(void* self)
{
    parser_free_buffers((ParserBuffers*)self);
}

typeof(__{{ prefix }}__t_.{{ start_type }}) {{ prefix }}_parse_len(void* error_ctx, void* buffers_, const char* input, uint32_t input_len)
{
    NeoastInput* lexer_input = input_new_from_buffer(input, input_len);

    typeof(__{{ prefix }}__t_.{{ start_type }}) ret;
    ret = {{ prefix }}_parse_input(error_ctx, buffers_, lexer_input);

    input_free(lexer_input);
    return ret;
}

typeof(__{{ prefix }}__t_.{{ start_type }}) {{ prefix }}_parse(void* error_ctx, void* buffers_, const char* input)
{
    return {{ prefix }}_parse_len(error_ctx, buffers_, input, strlen(input));
}

typeof(__{{ prefix }}__t_.{{ start_type }}) {{ prefix }}_parse_input(void* error_ctx, void* buffers_, NeoastInput* input)
{
    ParserBuffers* buffers = (ParserBuffers*) buffers_;
    parser_reset_buffers(buffers);

    {{ lexer_new_inst }}

    int32_t output_idx = parser_parse_lr(
            &parser, error_ctx, {{ prefix }}_parsing_table,
            buffers, ll_inst, {{ lexer_next }});

    {{ lexer_del_inst }}

    if (output_idx < 0)
        return (typeof(__{{ prefix }}__t_.{{ start_type }}))0;

    return (({{ struct_name }}*)buffers->value_table)[output_idx].value.{{ start_type }};
}

/************************************ BOTTOM *************************************/
{{ bottom }}
)", source_data);
}
