
#ifndef NEOAST_LARK_CODEGEN_COMMON_H
#define NEOAST_LARK_CODEGEN_COMMON_H

#include <unordered_map>
#include <map>
#include <memory>
#include <string>
#include <exception>

namespace neoast
{
    template<typename K, typename V>
    using umap = std::unordered_map<K, V>;

    template<typename K, typename V>
    using map = std::map<K, V>;

    template<typename T>
    using up = std::unique_ptr<T>;

    class Exception : std::exception
    {
        std::string message;
    public:
        explicit Exception(std::string message_) : message(std::move(message_)){}
        const char * what() const noexcept override
        {
            return message.c_str();
        }
    };
}


#endif //NEOAST_LARK_CODEGEN_COMMON_H
