#ifndef NEOAST_CG_UTIL_H
#define NEOAST_CG_UTIL_H

#include <string>
#include <sstream>
#include <codegen/codegen.h>
#include <memory>

struct Options {
    // Should we dump the table
    bool debug_table = false;
    bool no_warn_builtin = false;
    std::string track_position_type;
    std::string debug_ids;
    std::string prefix = "neoast";
    std::string lexing_error_cb;
    std::string syntax_error_cb;
    std::string lexer_file;
    parser_t parser_type = LALR_1; // LALR(1) or CLR(1)

    int parsing_stack_n = 1024;
    int max_tokens = 1024;

    void handle(const KeyVal* option);
};

class Exception : std::exception
{
    std::string what_;

public:
    explicit Exception(std::string what) : what_(std::move(what)) {}
    const char * what() const noexcept override { return what_.c_str(); }
};

class ASTException : public Exception
{
    TokenPosition position_;

public:
    explicit ASTException(TokenPosition position, std::string what)
    : position_(position), Exception(std::move(what)) {}
    const TokenPosition* position() const noexcept { return &position_; }
};

template<typename T_,
        typename... Args>
std::string variadic_string(const char* format,
                            T_ arg1,
                            Args... args)
{
    int size = std::snprintf(nullptr, 0, format, arg1, args...);
    if(size <= 0)
    {
        throw Exception( "Error during formatting: " + std::string(format));
    }
    size += 1;

    auto buf = std::unique_ptr<char[]>(new char[size]);
    std::snprintf(buf.get(), size, format, arg1, args...);

    return std::string(buf.get(), buf.get() + size - 1); // We don't want the '\0' inside
}


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
            ;
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
                uint8_t c = std::stoul(i.substr(13), nullptr, 16);
                os << variadic_string(" '%c'", c);
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
