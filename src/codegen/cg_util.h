#ifndef NEOAST_CG_UTIL_H
#define NEOAST_CG_UTIL_H

#include <string>
#include <sstream>
#include <codegen/codegen.h>
#include <util/util.h>
#include <memory>

struct Options {
    // Should we dump the table
    bool annotate_line = true;
    std::string track_position_type = "TokenPosition";
    std::string debug_ids;
    std::string prefix = "neoast";
    std::string lexing_error_cb;
    std::string syntax_error_cb;
    parser_t parser_type = LALR_1; // LALR(1) or CLR(1)

    int parsing_stack_n = 1024;
    int max_tokens = 1024;

    void handle(const KeyVal* option);
};

class ASTException : public Exception
{
    TokenPosition position_;

public:
    explicit ASTException(TokenPosition position, std::string what)
    : position_(position), Exception(std::move(what)) {}
    const TokenPosition* position() const noexcept { return &position_; }
};


template<typename T>
void put_enum(int start, const T& iterable, std::ostream& os,
              int skip_start = 0)
{
    os << "enum\n{\n";
    bool is_first = true;
    int _i = 0;
    for (const std::string& i : iterable)
    {
        if (_i < skip_start)
        {
        }
        else if (is_first)
        {
            is_first = false;
            os << variadic_string("    %s = %d,\n", i.c_str(), start);
        }
        else
        {
            os << variadic_string("    %s, // %d 0x%03X", i.c_str(), start, start);
            if (i.substr(0, 13) == "ASCII_CHAR_0x")
            {
                char ascii = get_ascii_from_name(i.c_str());
                const char* printable = get_ascii_printable(ascii);
                if (printable) os << " '" << printable << "'";
                else os << variadic_string(" '%c'", ascii);
            }

            os << "\n";
        }

        _i++;
        start++;
    }

    os << "};\n\n";
}

std::vector<std::pair<int, int>> mark_redzones(const std::string& code);

#endif //NEOAST_CG_UTIL_H
