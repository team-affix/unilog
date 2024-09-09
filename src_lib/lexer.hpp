#ifndef LEXER_H
#define LEXER_H

#include <list>
#include <string>
#include <filesystem>
#include <iterator>
#include <variant>

namespace unilog
{

    struct list_open
    {
    };

    struct list_close
    {
    };

    struct command
    {
        std::string m_text;
    };

    struct variable
    {
        std::string m_identifier;
    };

    struct atom
    {
        std::string m_text;
    };

    // Comparisons for lexeme types.
    bool operator==(const list_open &a_lhs, const list_open &a_rhs);
    bool operator==(const list_close &a_lhs, const list_close &a_rhs);
    bool operator==(const command &a_lhs, const command &a_rhs);
    bool operator==(const variable &a_lhs, const variable &a_rhs);
    bool operator==(const atom &a_lhs, const atom &a_rhs);

    typedef std::variant<
        list_open,
        list_close,
        command,
        variable,
        atom>
        lexeme;

    // General extractor for lexemes.
    std::istream &operator>>(std::istream &a_istream, lexeme &a_lexeme);

}

#endif
