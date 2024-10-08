#include <string>
#include <regex>
#include <stdexcept>
#include <cctype>
#include "lexer.hpp"

std::istream &escape(std::istream &a_istream, char &a_char)
{
    char l_char;
    a_istream.get(l_char);

    switch (l_char)
    {
    case '0':
    {
        a_char = '\0';
    }
    break;
    case 'a':
    {
        a_char = '\a';
    }
    break;
    case 'b':
    {
        a_char = '\b';
    }
    break;
    case 't':
    {
        a_char = '\t';
    }
    break;
    case 'n':
    {
        a_char = '\n';
    }
    break;
    case 'v':
    {
        a_char = '\v';
    }
    break;
    case 'f':
    {
        a_char = '\f';
    }
    break;
    case 'r':
    {
        a_char = '\r';
    }
    break;
    case 'x':
    {
        char l_upper_hex_digit;
        char l_lower_hex_digit;

        if (!a_istream.get(l_upper_hex_digit))
            break;
        if (!a_istream.get(l_lower_hex_digit))
            break;

        if (std::isxdigit(l_upper_hex_digit) == 0 || std::isxdigit(l_lower_hex_digit) == 0)
        {
            a_istream.setstate(std::ios::failbit);
            break;
        }

        int l_hex_value;

        std::stringstream l_ss;

        l_ss << std::hex << l_upper_hex_digit << l_lower_hex_digit;

        l_ss >> l_hex_value;

        a_char = static_cast<char>(l_hex_value);
    }
    break;
    default:
    {
        a_char = l_char;
    }
    break;
    }

    return a_istream;
}

namespace unilog
{

    // Comparison operators for lexeme types.
    bool operator==(const command &a_lhs, const command &a_rhs)
    {
        return a_lhs.m_text == a_rhs.m_text;
    }

    bool operator==(const scope_separator &a_lhs, const scope_separator &a_rhs)
    {
        return true;
    }

    bool operator==(const list_separator &a_lhs, const list_separator &a_rhs)
    {
        return true;
    }

    bool operator==(const list_open &a_lhs, const list_open &a_rhs)
    {
        return true;
    }

    bool operator==(const list_close &a_lhs, const list_close &a_rhs)
    {
        return true;
    }

    bool operator==(const variable &a_lhs, const variable &a_rhs)
    {
        return a_lhs.m_identifier == a_rhs.m_identifier;
    }

    bool operator==(const quoted_atom &a_lhs, const quoted_atom &a_rhs)
    {
        return a_lhs.m_text == a_rhs.m_text;
    }

    bool operator==(const unquoted_atom &a_lhs, const unquoted_atom &a_rhs)
    {
        return a_lhs.m_text == a_rhs.m_text;
    }

    bool is_command_indicator_char(int c)
    {
        return c == '!';
    }

    bool is_scope_separator_indicator_char(int c)
    {
        return c == ':';
    }

    bool is_list_separator_indicator_char(int c)
    {
        return c == '|';
    }

    bool is_list_open_indicator_char(int c)
    {
        return c == '[';
    }

    bool is_list_close_indicator_char(int c)
    {
        return c == ']';
    }

    bool is_variable_indicator_char(int c)
    {
        return isupper(c) ||
               c == '_';
    }

    bool is_quoted_atom_indicator_char(int c)
    {
        // single OR double-quote.
        return c == '\'' ||
               c == '\"';
    }

    bool is_unquoted_atom_indicator_char(int c)
    {
        return !is_command_indicator_char(c) &&
               !is_scope_separator_indicator_char(c) &&
               !is_list_separator_indicator_char(c) &&
               !is_list_open_indicator_char(c) &&
               !is_list_close_indicator_char(c) &&
               !is_variable_indicator_char(c) &&
               !is_quoted_atom_indicator_char(c);
    }

    bool is_command_text_char(int c)
    {
        return isalnum(c);
    }

    bool is_variable_text_char(int c)
    {
        return isalnum(c) ||
               c == '_';
    }

    bool is_unquoted_atom_text_char(int c)
    {
        return !is_command_indicator_char(c) &&
               !is_scope_separator_indicator_char(c) &&
               !is_list_separator_indicator_char(c) &&
               !is_list_open_indicator_char(c) &&
               !is_list_close_indicator_char(c) &&
               !is_quoted_atom_indicator_char(c) &&
               std::isspace(c) == 0 &&
               c != std::istream::traits_type::eof();
    }

    std::istream &consume_whitespace(std::istream &a_istream)
    {
        char l_char;

        // Extract character (read until first non-whitespace)
        while (std::isspace(a_istream.peek()) != 0 && a_istream.get(l_char))
            ;

        return a_istream;
    }

    std::istream &operator>>(std::istream &a_istream, command &a_command)
    {
        // save streamposition, restore on failure
        std::streampos l_initial_sp = a_istream.tellg();

        ////////////////////////////////////
        //////// INDICATOR SECTION /////////
        ////////////////////////////////////

        // consume until non-whitespace char
        consume_whitespace(a_istream);

        if (!is_command_indicator_char(a_istream.get()))
        {
            a_istream.clear();
            a_istream.setstate(std::ios::failbit);
            // restore streampos
            a_istream.seekg(l_initial_sp);
            return a_istream;
        }

        ////////////////////////////////////
        /////////// TEXT SECTION ///////////
        ////////////////////////////////////

        // clear the text in the resulting command
        a_command.m_text.clear();

        char l_char;

        while (
            // conditions for consumption
            is_command_text_char(a_istream.peek()) &&
            // Consume char now
            a_istream.get(l_char))
        {
            a_command.m_text.push_back(l_char);
        }

        return a_istream;
    }

    std::istream &operator>>(std::istream &a_istream, scope_separator &a_scope_separator)
    {
        // save streamposition, restore on failure
        std::streampos l_initial_sp = a_istream.tellg();

        ////////////////////////////////////
        //////// INDICATOR SECTION /////////
        ////////////////////////////////////

        // consume until non-whitespace char
        consume_whitespace(a_istream);

        if (!is_scope_separator_indicator_char(a_istream.get()))
        {
            a_istream.clear();
            a_istream.setstate(std::ios::failbit);
            // restore streampos
            a_istream.seekg(l_initial_sp);
            return a_istream;
        }

        return a_istream;
    }

    std::istream &operator>>(std::istream &a_istream, list_separator &a_list_separator)
    {
        // save streamposition, restore on failure
        std::streampos l_initial_sp = a_istream.tellg();

        ////////////////////////////////////
        //////// INDICATOR SECTION /////////
        ////////////////////////////////////

        // consume until non-whitespace char
        consume_whitespace(a_istream);

        if (!is_list_separator_indicator_char(a_istream.get()))
        {
            a_istream.clear();
            a_istream.setstate(std::ios::failbit);
            // restore streampos
            a_istream.seekg(l_initial_sp);
            return a_istream;
        }

        return a_istream;
    }

    std::istream &operator>>(std::istream &a_istream, list_open &a_list_open)
    {
        // save streamposition, restore on failure
        std::streampos l_initial_sp = a_istream.tellg();

        ////////////////////////////////////
        //////// INDICATOR SECTION /////////
        ////////////////////////////////////

        // consume until non-whitespace char
        consume_whitespace(a_istream);

        if (!is_list_open_indicator_char(a_istream.get()))
        {
            a_istream.clear();
            a_istream.setstate(std::ios::failbit);
            // restore streampos
            a_istream.seekg(l_initial_sp);
            return a_istream;
        }

        return a_istream;
    }

    std::istream &operator>>(std::istream &a_istream, list_close &a_list_close)
    {
        // save streamposition, restore on failure
        std::streampos l_initial_sp = a_istream.tellg();

        ////////////////////////////////////
        //////// INDICATOR SECTION /////////
        ////////////////////////////////////

        // consume until non-whitespace char
        consume_whitespace(a_istream);

        if (!is_list_close_indicator_char(a_istream.get()))
        {
            a_istream.clear();
            a_istream.setstate(std::ios::failbit);
            // restore streampos
            a_istream.seekg(l_initial_sp);
            return a_istream;
        }

        return a_istream;
    }

    std::istream &operator>>(std::istream &a_istream, variable &a_variable)
    {
        // save streamposition, restore on failure
        std::streampos l_initial_sp = a_istream.tellg();

        ////////////////////////////////////
        //////// INDICATOR SECTION /////////
        ////////////////////////////////////

        // consume until non-whitespace char
        consume_whitespace(a_istream);

        if (!is_variable_indicator_char(a_istream.peek()))
        {
            a_istream.clear();
            a_istream.setstate(std::ios::failbit);
            // restore streampos
            a_istream.seekg(l_initial_sp);
            return a_istream;
        }

        ////////////////////////////////////
        /////////// TEXT SECTION ///////////
        ////////////////////////////////////

        a_variable.m_identifier.clear();

        char l_char;

        // Variable lexemes must only contain alphanumeric chars,
        //     beginning with an upper-case letter or underscore.

        while (
            // conditions for consumption
            is_variable_text_char(a_istream.peek()) &&
            // Consume char now
            a_istream.get(l_char))
        {
            a_variable.m_identifier.push_back(l_char);
        }

        return a_istream;
    }

    std::istream &operator>>(std::istream &a_istream, quoted_atom &a_quoted_atom)
    {
        // save streamposition, restore on failure
        std::streampos l_initial_sp = a_istream.tellg();

        ////////////////////////////////////
        //////// INDICATOR SECTION /////////
        ////////////////////////////////////

        // consume until non-whitespace char
        consume_whitespace(a_istream);

        // save the type of quotation. then we can match for closing quote.
        int l_quote_char = a_istream.get();

        if (!is_quoted_atom_indicator_char(l_quote_char))
        {
            a_istream.clear();
            a_istream.setstate(std::ios::failbit);
            // restore streampos
            a_istream.seekg(l_initial_sp);
            return a_istream;
        }

        ////////////////////////////////////
        /////////// TEXT SECTION ///////////
        ////////////////////////////////////

        a_quoted_atom.m_text.clear();

        char l_char;

        // The input was a quote character. Thus we should scan until the closing quote
        //     to produce a valid lexeme.
        while (
            // Consume char now
            a_istream.get(l_char))
        {
            if (l_char == l_quote_char)
                break;

            if (l_char == '\\')
                escape(a_istream, l_char);

            a_quoted_atom.m_text.push_back(l_char);
        }

        return a_istream;
    }

    std::istream &operator>>(std::istream &a_istream, unquoted_atom &a_unquoted_atom)
    {
        // save streamposition, restore on failure
        std::streampos l_initial_sp = a_istream.tellg();

        ////////////////////////////////////
        //////// INDICATOR SECTION /////////
        ////////////////////////////////////

        // consume until non-whitespace char
        consume_whitespace(a_istream);

        if (!is_unquoted_atom_indicator_char(a_istream.peek()))
        {
            a_istream.clear();
            a_istream.setstate(std::ios::failbit);
            // restore streampos
            a_istream.seekg(l_initial_sp);
            return a_istream;
        }

        ////////////////////////////////////
        /////////// TEXT SECTION ///////////
        ////////////////////////////////////

        a_unquoted_atom.m_text.clear();

        char l_char;

        // The input was unquoted. Thus it should be treated as text,
        //     and we must terminate the text at a lexeme separator character OR
        //     when we reach EOF.

        // NOTE: Since we may simply hit EOF before any lexeme separator, we
        //     should not consume the EOF so as to prevent setting the failbit.
        while (
            // conditions for consumption
            is_unquoted_atom_text_char(a_istream.peek()) &&
            // Get the char now
            a_istream.get(l_char))
        {
            a_unquoted_atom.m_text.push_back(l_char);
        }

        return a_istream;
    }

    std::istream &operator>>(std::istream &a_istream, atom &a_atom)
    {
        // case: quoted_atom
        {
            quoted_atom l_result;
            if (a_istream >> l_result)
            {
                a_atom = l_result;
                return a_istream;
            }
            a_istream.clear();
        }

        // case: unquoted_atom
        {
            unquoted_atom l_result;
            if (a_istream >> l_result)
            {
                a_atom = l_result;
                return a_istream;
            }
            a_istream.clear();
        }

        a_istream.setstate(std::ios::failbit);

        return a_istream;
    }

    std::istream &operator>>(std::istream &a_istream, lexeme &a_lexeme)
    {
        /// NOTE TO SELF IN FUTURE
        /// the purpose of the three bits:
        ///     eofbit failbit badbit
        ///     eofbit:  reaching the end of file, does NOT indicate failure to extract.
        ///     failbit: triggered when attempting to read past EOF, and can be set otherwise. indicates FAILURE to extract
        ///     badbit:  severe stream error (maybe outside of program's control).

        /// NOTES ON istream_iterator
        ///     istream_iterator will only terminate extraction once an extraction FAILS. in other words, simply enabling eofbit is NOT
        ///     enough to terminate extraction. failbit is required to terminate, I am NOT sure if eofbit is required however.

        // case: command
        {
            command l_result;
            if (a_istream >> l_result)
            {
                a_lexeme = l_result;
                return a_istream;
            }
            a_istream.clear();
        }

        // case: scope_separator
        {
            scope_separator l_result;
            if (a_istream >> l_result)
            {
                a_lexeme = l_result;
                return a_istream;
            }
            a_istream.clear();
        }

        // case: list_separator
        {
            list_separator l_result;
            if (a_istream >> l_result)
            {
                a_lexeme = l_result;
                return a_istream;
            }
            a_istream.clear();
        }

        // case: list_open
        {
            list_open l_result;
            if (a_istream >> l_result)
            {
                a_lexeme = l_result;
                return a_istream;
            }
            a_istream.clear();
        }

        // case: list_close
        {
            list_close l_result;
            if (a_istream >> l_result)
            {
                a_lexeme = l_result;
                return a_istream;
            }
            a_istream.clear();
        }

        // case: variable
        {
            variable l_result;
            if (a_istream >> l_result)
            {
                a_lexeme = l_result;
                return a_istream;
            }
            a_istream.clear();
        }

        // case: atom
        {
            atom l_result;
            if (a_istream >> l_result)
            {
                a_lexeme = l_result;
                return a_istream;
            }
            a_istream.clear();
        }

        a_istream.setstate(std::ios::failbit);

        return a_istream;
    }
}
