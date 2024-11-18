#ifndef PARSER_HPP
#define PARSER_HPP

#include <memory>
#include <variant>
#include <istream>
#include <SWI-Prolog.h>

namespace unilog
{
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

    using statement = std::variant<
        axiom_statement,
        guide_statement,
        infer_statement,
        refer_statement>;

    std::istream &operator>>(std::istream &a_istream, statement &a_statement);

}

#endif
