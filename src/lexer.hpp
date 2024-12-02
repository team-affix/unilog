#ifndef LEXER_HPP
#define LEXER_HPP

#include <list>
#include <string>
#include <filesystem>
#include <iterator>
#include <variant>

#define ERR_MSG_CLOSING_QUOTE "Error: no closing quote"
#define ERR_MSG_INVALID_LEXEME "Error: invalid lexeme"

namespace unilog
{

    struct eol
    {
    };

    struct list_separator
    {
    };

    struct list_open
    {
    };

    struct list_close
    {
    };

    struct variable
    {
        std::string m_identifier;
    };

    struct atom
    {
        std::string m_text;
    };

    /////////////////////////////////

    // Comparisons for lexeme types.
    bool operator==(const eol &a_lhs, const eol &a_rhs);
    bool operator==(const list_separator &a_lhs, const list_separator &a_rhs);
    bool operator==(const list_open &a_lhs, const list_open &a_rhs);
    bool operator==(const list_close &a_lhs, const list_close &a_rhs);
    bool operator==(const variable &a_lhs, const variable &a_rhs);
    bool operator==(const atom &a_lhs, const atom &a_rhs);

    using lexeme = std::variant<
        eol,
        list_separator,
        list_open,
        list_close,
        variable,
        atom>;

    std::istream &operator>>(std::istream &a_istream, lexeme &a_lexeme);

}

#endif
