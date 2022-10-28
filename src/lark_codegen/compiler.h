
#ifndef NEOAST_COMPILER_H
#define NEOAST_COMPILER_H

#include <lark_codegen/ast.h>

#include <string>
#include <unordered_map>
#include <functional>
#include <unordered_set>

#include "input_file.h"
#include "types.h"
#include "parsergen/canonical_collection.h"

namespace neoast
{
    class Compiler;

    class Token : public LRItem
    {
    public:
        const ast::Token& decl;
        const ast::Token* type_decl;

        bool generates_rule;
        bool use_regex_for_c_name = false;
        std::string regex;

        Token(const ast::Token& decl_,
              Type* type,
              std::string name_,
              std::string regex_);
        explicit Token(const ast::Token& decl_,
                       const ast::Token* type_decl_);
    };

    TokenPosition merge_positions(const TokenPosition& t1, const TokenPosition& t2);
    TokenPosition get_position_from_expr(const ast::Expr* expr);
    TokenPosition get_position_from_expr(const std::list<up<ast::Expr>>& exprs);

    Type* get_expr_type(const ast::Expr* expr, Compiler& c);

    struct BaseGrammar : public LRItem
    {
        explicit BaseGrammar(const TokenPosition& position_,
                             std::string name_,
                             Type* type);
    };

    struct Grammar : public BaseGrammar
    {
        const ast::Grammar& decl;
        explicit Grammar(const ast::Grammar& decl_);
    };

    class ProductionTree
    {
    public:
        using Branch = std::list<LRItem*>;

        ProductionTree();
        ProductionTree(const ProductionTree &tree) = default;
        ProductionTree(ProductionTree &&tree) = default;

        std::list<Branch>::iterator begin();
        std::list<Branch>::iterator end();
        std::list<Branch>::const_iterator begin() const;
        std::list<Branch>::const_iterator end() const;

        void branch(const ProductionTree& left_tree,
                    const ProductionTree& right_tree);

        void push(const ProductionTree& tree);

        void push(LRItem* item);

    private:
        std::list<Branch> branches;
    };

    struct LRProduction
    {
        LRProduction(
                Compiler& c,
                BaseGrammar* return_token_,
                std::list<LRItem*> items,
                const ast::Token* action = nullptr,
                const ast::Token* alias_ = nullptr);

        BaseGrammar* return_token;
        std::list<LRItem*> production;

        const ast::Token* action;
        const ast::Token* alias;

        GrammarRule get_rule() const;

    private:
        up<tok_t[]> grammar_expanded;
    };

    class Compiler
    {
    public:
        Compiler();

        up<parsergen::CanonicalCollection> cc;
        up<uint8_t[]> precedence_table;
        up<uint32_t[]> parsing_table;
        std::map<int, int> precedence_mapping;

        umap<std::string, up<Type>> types;
        map<std::string, Token> tokens;
        map<std::string, Grammar> grammars;
        std::list<LRProduction> productions;
        map<std::string, BaseGrammar> generated_grammars;
        std::list<AstVector> generated_vectors;

        umap<std::string, std::string> options;

        int compile(const std::string& file_path);

        BaseGrammar& generate_grammar(const TokenPosition& position, Type* type);

        GlobalContext context;

    private:
        int error;

        // top level AST
        // Will be expanded by imports
        ast::CompilerCompiler* ast = nullptr;

        std::unordered_set<const ast::Stmt*> processed_imports;
        std::vector<InputFile> file_inputs;
        std::vector<up<ast::CompilerCompiler>> ast_inputs;

        std::list<ast::Token> generated_tokens;
        std::list<AstEnum> generated_enums;

        int num_reserved_tokens;
        umap<std::string, int> reserved_tokens;

        // Stage 1 items
        struct
        {
            const ast::Token* start = nullptr;
            std::list<const ast::Token*> left;
            std::list<const ast::Token*> right;
            std::list<const ast::Token*> top;
            std::list<const ast::Token*> include;
            std::list<const ast::Token*> bottom;
        } s1;

        ast::CompilerCompiler* load_file(const std::string& path);

        void clear();

        /**
         * Collects imports and inserts
         * items into the AST
         */
        void do_s0();

        /**
         * Processes all static statements
         * with no dependencies
         */
        void do_s1();

        /**
         * Create all pure lexer tokens defined explicitly
         * (Not those sitting in grammar rules)
         */
        void do_s2();

        /**
         * Add builtin-types, defined types,
         * and resolve inheritance
         */
        void do_s3();

        /**
         * Move through grammar productions
         * Build up type members, type aliases for classes
         * Build up enumerate names for enum
         */
        void do_s4();

        /**
         * Resolve grammar into true LR productions
         */
        void do_s5();

        /**
         * Give all LR items an ID
         */
        void do_s6();

        /**
         * Build and resolve the canonical collection
         */
        void do_s7();

        ProductionTree resolve_expr(const ast::Expr* expr);
        ProductionTree resolve_expr(const std::list<up<ast::Expr>>& exprs);

        // Compiler helper functions

        void do_stage(const std::function<void()>& stage_cb, int error_code);

        static std::string get_reserved_token(int token_id);
        bool extract_token_name_from_literal(const std::string& literal, std::string& token_name);
    };
}

#endif //NEOAST_COMPILER_H
