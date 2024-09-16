#ifndef LEXER_H
#define LEXER_H

#include <list>
#include <string>
#include <filesystem>
#include <iterator>
#include <variant>

namespace unilog
{

    struct command
    {
        std::string m_text;
    };

    /////////////////////////////////
    // SCOPING

    struct scope_separator
    {
    };

    /////////////////////////////////

    /////////////////////////////////
    // PROLOG

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

    struct quoted_atom
    {
        std::string m_text;
    };

    struct unquoted_atom
    {
        std::string m_text;
    };

    typedef std::variant<
        quoted_atom,
        unquoted_atom>
        atom;

    /////////////////////////////////

    // Comparisons for lexeme types.
    bool operator==(const command &a_lhs, const command &a_rhs);
    bool operator==(const scope_separator &a_lhs, const scope_separator &a_rhs);
    bool operator==(const list_separator &a_lhs, const list_separator &a_rhs);
    bool operator==(const list_open &a_lhs, const list_open &a_rhs);
    bool operator==(const list_close &a_lhs, const list_close &a_rhs);
    bool operator==(const variable &a_lhs, const variable &a_rhs);
    bool operator==(const unquoted_atom &a_lhs, const unquoted_atom &a_rhs);
    bool operator==(const quoted_atom &a_lhs, const quoted_atom &a_rhs);

    typedef std::variant<
        command,
        scope_separator,
        list_separator,
        list_open,
        list_close,
        variable,
        atom>
        lexeme;

    std::istream &operator>>(std::istream &a_istream, command &a_command);

    std::istream &operator>>(std::istream &a_istream, scope_separator &a_command);
    std::istream &operator>>(std::istream &a_istream, list_separator &a_command);

    std::istream &operator>>(std::istream &a_istream, list_open &a_list_open);
    std::istream &operator>>(std::istream &a_istream, list_close &a_list_close);
    std::istream &operator>>(std::istream &a_istream, variable &a_variable);
    std::istream &operator>>(std::istream &a_istream, quoted_atom &a_quoted_atom);
    std::istream &operator>>(std::istream &a_istream, unquoted_atom &a_unquoted_atom);
    std::istream &operator>>(std::istream &a_istream, atom &a_atom);
    std::istream &operator>>(std::istream &a_istream, lexeme &a_lexeme);

}

#endif
