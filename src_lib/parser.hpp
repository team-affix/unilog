#ifndef PARSER_HPP
#define PARSER_HPP

#include <memory>
#include <variant>
#include <istream>
#include <list>

#include "lexer.hpp"

namespace unilog
{
    // cons cell (defined for unilog terms)
    template <typename T>
    struct cons
    {
        const T m_car;
        std::unique_ptr<const T> m_cdr;
        cons(const T &a_car = T(), const T *a_cdr = nullptr) : m_car(a_car), m_cdr(a_cdr) {}
    };

    // self-referencial type requires a formal declaration
    struct term
    {
        std::variant<atom, variable, std::list<term>> m_variant;
    };

    struct axiom_statement
    {
        atom m_tag;
        term m_theorem;
    };

    struct guide_statement
    {
        term m_tag;
        std::list<variable> m_args;
        term m_guide;
    };

    struct infer_statement
    {
        atom m_tag;
        term m_theorem;
        term m_guide;
    };

    struct refer_statement
    {
        atom m_tag;
        quoted_atom m_file_path;
    };

    bool operator==(const term &a_lhs, const term &a_rhs);
    bool operator==(const axiom_statement &a_lhs, const axiom_statement &a_rhs);
    bool operator==(const guide_statement &a_lhs, const guide_statement &a_rhs);
    bool operator==(const infer_statement &a_lhs, const infer_statement &a_rhs);
    bool operator==(const refer_statement &a_lhs, const refer_statement &a_rhs);

    using statement = std::variant<
        axiom_statement,
        guide_statement,
        infer_statement,
        refer_statement>;

    std::istream &operator>>(std::istream &a_istream, term &a_term);
    std::istream &operator>>(std::istream &a_istream, axiom_statement &a_axiom_statement);
    std::istream &operator>>(std::istream &a_istream, guide_statement &a_guide_statement);
    std::istream &operator>>(std::istream &a_istream, infer_statement &a_infer_statement);
    std::istream &operator>>(std::istream &a_istream, refer_statement &a_refer_statement);

}

#endif
