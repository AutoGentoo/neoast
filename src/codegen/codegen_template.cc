//
// Created by tumbar on 7/17/21.
//

#include <codegen/codegen_impl.h>
#include <inja/inja.hpp>

void CodeGenImpl::write_header(std::ostream &os) const
{
    std::ostringstream os_tokens;
    std::ostringstream os_lexer;
    put_enum(NEOAST_ASCII_MAX + 1, tokens, os_tokens);
    lexer->put_header(os_lexer);

    inja::json header_data;
    header_data["prefix"] = options.prefix;
    header_data["tokens"] = os_tokens.str();
    header_data["lexer"] = os_lexer.str();

    // TODO Declare parsing functions
    os << inja::render(
            R"(#ifndef __NEOAST_{{ upper(prefix) }}_H__
#define __NEOAST_{{ upper(prefix) }}_H__

#ifdef __cplusplus
extern "C" {
#endif

#ifdef __NEOAST_GET_TOKENS__
{{ tokens }}
#endif // __NEOAST_GET_TOKENS__

{{ lexer }}

#ifdef __cplusplus
}
#endif

#endif // __NEOAST_{{ upper(prefix) }}_H__
)", header_data);
}

void CodeGenImpl::write_source(std::ostream &os) const
{
    // Act as a preprocessor and put the external header
    std::ostringstream os_header_preprocessor;
    write_header(os_header_preprocessor);

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
    inja::json destructors_json;
    for (const auto &iter : destructors)
    {
        destructors_json[iter.first] = iter.second->get_complex(options, {iter.first}, "self", "", true);
    }

    inja::json destructor_table_json;
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
    source_data["top"] = top->get_simple();
    source_data["bottom"] = bottom->get_simple();
    source_data["union"] = union_->get_simple();
    source_data["union_name"] = CODEGEN_UNION;
    source_data["struct_name"] = CODEGEN_STRUCT;
    source_data["start_type"] = start_type;

    /* Destructors */
    source_data["destructors"] = destructors_json;
    source_data["destructor_table"] = destructor_table_json;

    /* Lexer parameters */
    source_data["lexer"] = os_lexer.str();
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
    source_data["lexer_opts"] = options.lexer_opts;
    source_data["track_position"] = options.lexer_opts & LEXER_OPT_TOKEN_POS;
    source_data["prefix"] = options.prefix;
    source_data["max_tokens"] = options.max_tokens;
    source_data["parsing_stack_n"] = options.parsing_stack_n;

    /* Generated */
    source_data["tokens"] = tokens;
    source_data["ascii_mappings"] = os_ascii_mappings.str();
    source_data["parsing_table"] = os_parsing_table.str();

    os << inja::render(
            R"(#define __NEOAST_GET_TOKENS__
#define __NEOAST_GET_STATES__

#include <neoast.h>
#include <string.h>

{{ include_header }}

/************************************ TOP ***************************************/
{{ top }}

/************************ NEOAST Union/Struct definition ************************/
typedef union { {{ union }} } {{ union_name }};
{%- if track_position -%}
typedef struct {
    {{ union_name }} value;
    TokenPosition position;
} {{ struct_name }};
{%- else -%}
typedef struct {
    {{ union_name }} value;
} {{ struct_name }};
{%- endif -%}

/********************************* DESTRUCTORS **********************************/
{% for key, code in destructors %}
static void
{{ key }}_destructor({{ union_name }}* self) { {{- code -}} }
{% endfor %}

// Destructor table
static const
parser_destructor __neoast_token_destructors[] = {
{%- for entry in destructor_table %}
    {{ entry.function }}, // {{ entry.token_name -}}
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

static GrammarParser parser = {
        .grammar_n = {{ grammar_n }},
        .ascii_mappings = __neoast_ascii_mappings,
        .grammar_rules = __neoast_grammar_rules,
        .token_names = __neoast_token_names,
        .destructors = __neoast_token_destructors,
        .parser_error = {{ parser_error }},
        .token_n = TOK_AUGMENT - NEOAST_ASCII_MAX,
        .action_token_n = {{ action_n }},
        .lexer_opts = (lexer_option_t) {{ lexer_opts }}
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
{%- if track_position %}            offsetof({{ struct_name }}, position)
{%- else %}            sizeof({{ struct_name }})
{%- endif %}    );
}

void {{ prefix }}_free_buffers(void* self)
{
    parser_free_buffers((ParserBuffers*)self);
}

extern {{ union_name }} t;
typeof(t.{{ start_type }}) {{ prefix }}_parse(void* buffers_, const char* input)
{
    ParserBuffers* buffers = (ParserBuffers*) buffers_;
    parser_reset_buffers(buffers);

    uint64_t input_len = strlen(input);
    {{ lexer_new_inst }};

    int32_t output_idx = parser_parse_lr(
            &parser, GEN_parsing_table,
            buffers, ll_inst, {{ lexer_next }});

    {{ lexer_del_inst }};

    if (output_idx < 0)
        return (typeof(t.{{start_type}}))0;

    return (({{ union_name }}*)buffers->value_table)[output_idx].{{ start_type }};
}

typeof(t.{{ start_type }}) {{ prefix }}_parse_len(void* buffers_, const char* input, uint32_t input_len)
{
    ParserBuffers* buffers = (ParserBuffers*) buffers_;
    parser_reset_buffers(buffers);

    {{ lexer_new_inst }};

    int32_t output_idx = parser_parse_lr(
            &parser, GEN_parsing_table,
            buffers, ll_inst, {{ lexer_next }});

    {{ lexer_del_inst }};

    if (output_idx < 0)
        return (typeof(t.{{start_type}}))0;

    return (({{ union_name }}*)buffers->value_table)[output_idx].{{ start_type }};
}

/************************************ BOTTOM *************************************/
{{ bottom }}
)", source_data);
}
