#ifndef PARSER_HPP
#define PARSER_HPP

#include <variant>
#include <list>
#include <istream>

#include "lexer.hpp"

namespace unilog
{

    // self-referencial type requires a formal declaration
    struct prolog_expression
    {
        std::variant<atom, variable, std::list<prolog_expression>> m_variant;
    };

    struct axiom_statement
    {
        atom m_tag;
        prolog_expression m_theorem;
    };

    struct guide_statement
    {
        prolog_expression m_tag;
        std::list<variable> m_args;
        prolog_expression m_guide;
    };

    struct infer_statement
    {
        atom m_tag;
        prolog_expression m_theorem;
        prolog_expression m_guide;
    };

    struct refer_statement
    {
        atom m_tag;
        quoted_atom m_file_path;
    };

    bool operator==(const prolog_expression &a_lhs, const prolog_expression &a_rhs);
    bool operator==(const axiom_statement &a_lhs, const axiom_statement &a_rhs);
    bool operator==(const guide_statement &a_lhs, const guide_statement &a_rhs);
    bool operator==(const infer_statement &a_lhs, const infer_statement &a_rhs);
    bool operator==(const refer_statement &a_lhs, const refer_statement &a_rhs);

    using statement = std::variant<
        axiom_statement,
        guide_statement,
        infer_statement,
        refer_statement>;

    std::istream &operator>>(std::istream &a_istream, prolog_expression &a_prolog_expression);
    std::istream &operator>>(std::istream &a_istream, axiom_statement &a_axiom_statement);
    std::istream &operator>>(std::istream &a_istream, guide_statement &a_guide_statement);
    std::istream &operator>>(std::istream &a_istream, infer_statement &a_infer_statement);
    std::istream &operator>>(std::istream &a_istream, refer_statement &a_refer_statement);

}

#endif
