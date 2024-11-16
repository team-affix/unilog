#ifndef PARSER_HPP
#define PARSER_HPP

#include <memory>
#include <variant>
#include <istream>
#include <map>
#include <SWI-Prolog.h>

namespace unilog
{
    // term_t cons_nil();
    // term_t cons_atom(const std::string &a_text);
    // term_t cons_variable(const std::string &a_identifier, std::map<std::string, term_t> &a_var_alist);
    // term_t cons_list(const term_t &a_car, const term_t &a_cdr);

    struct axiom_statement
    {
        term_t m_tag;
        term_t m_theorem;
    };

    struct guide_statement
    {
        term_t m_tag;
        term_t m_args;
        term_t m_guide;
    };

    struct infer_statement
    {
        term_t m_tag;
        term_t m_theorem;
        term_t m_guide;
    };

    struct refer_statement
    {
        term_t m_tag;
        term_t m_file_path;
    };

    bool operator==(const axiom_statement &a_lhs, const axiom_statement &a_rhs);
    bool operator==(const guide_statement &a_lhs, const guide_statement &a_rhs);
    bool operator==(const infer_statement &a_lhs, const infer_statement &a_rhs);
    bool operator==(const refer_statement &a_lhs, const refer_statement &a_rhs);

    using statement = std::variant<
        axiom_statement,
        guide_statement,
        infer_statement,
        refer_statement>;

    std::istream &extract_term_t(std::istream &a_istream, term_t &a_term_t, std::map<std::string, term_t> &a_var_alist);
    std::istream &operator>>(std::istream &a_istream, axiom_statement &a_axiom_statement);
    std::istream &operator>>(std::istream &a_istream, guide_statement &a_guide_statement);
    std::istream &operator>>(std::istream &a_istream, infer_statement &a_infer_statement);
    std::istream &operator>>(std::istream &a_istream, refer_statement &a_refer_statement);

}

#endif
