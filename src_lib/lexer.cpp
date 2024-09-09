#include <string>
#include <regex>
#include <stdexcept>
#include <cctype>
#include "lexer.hpp"

// Lexer special characters
#define COMMAND_CHAR '!'
#define LIST_OPEN_CHAR '['
#define LIST_CLOSE_CHAR ']'

#define UNNAMED_VARIABLE_CHAR '_'

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
            throw std::runtime_error("Expected hex character.");

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

    if (a_istream.fail())
    {
        throw std::runtime_error("Failed to escape character.");
    }

    return a_istream;
}

namespace unilog
{

    // Comparison operators for lexeme types.
    bool operator==(const list_open &a_lhs, const list_open &a_rhs)
    {
        return true;
    }
    bool operator==(const list_close &a_lhs, const list_close &a_rhs)
    {
        return true;
    }
    bool operator==(const command &a_lhs, const command &a_rhs)
    {
        return a_lhs.m_text == a_rhs.m_text;
    }
    bool operator==(const variable &a_lhs, const variable &a_rhs)
    {
        return a_lhs.m_identifier == a_rhs.m_identifier;
    }
    bool operator==(const atom &a_lhs, const atom &a_rhs)
    {
        return a_lhs.m_text == a_rhs.m_text;
    }

    bool is_quote(int c)
    {
        // single OR double-quote.
        return c == '\'' ||
               c == '\"';
    }

    bool is_lex_stop_character(int c)
    {
        return std::isspace(c) != 0 ||
               c == std::istream::traits_type::eof() ||
               c == LIST_OPEN_CHAR ||
               c == LIST_CLOSE_CHAR;
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

        char l_char;

        // Extract character (read until first non-whitespace)
        while (a_istream.get(l_char) && std::isspace(l_char) != 0)
            ;

        // If reading the character failed, just early return.
        //     Extracting lexeme failed.
        if (!a_istream.good())
            return a_istream;

        // 1. command `!`
        if (l_char == COMMAND_CHAR)
        {
            std::string l_text;

            // Command lexemes must only contain alphanumeric chars.

            while (
                // Conditions for consumption
                !is_lex_stop_character(a_istream.peek()) &&
                // Consume char now
                a_istream.get(l_char))
            {
                if (!isalnum(l_char))
                {
                    a_istream.setstate(std::ios::failbit);
                    throw std::runtime_error("Failed to parse command: non-alphanumeric character read.");
                }

                l_text.push_back(l_char);
            }

            a_lexeme = command{
                .m_text = l_text,
            };
        }
        // 2. list_open `[`
        else if (l_char == LIST_OPEN_CHAR)
        {
            a_lexeme = list_open{};
        }
        // 3. list_close `]`
        else if (l_char == LIST_CLOSE_CHAR)
        {
            a_lexeme = list_close{};
        }
        // 4. variable
        else if (isupper(l_char) || l_char == UNNAMED_VARIABLE_CHAR)
        {
            std::string l_identifier;

            l_identifier.push_back(l_char);

            // Variable lexemes must only contain alphanumeric chars,
            //     beginning with an upper-case letter or underscore.

            while (
                // Conditions for consumption
                !is_lex_stop_character(a_istream.peek()) &&
                // Consume char now
                a_istream.get(l_char))
            {
                if (!isalnum(l_char) && l_char != UNNAMED_VARIABLE_CHAR)
                {
                    a_istream.setstate(std::ios::failbit);
                    throw std::runtime_error("Failed to parse variable: non-alphanumeric, non-underscore (_) character read.");
                }

                l_identifier.push_back(l_char);
            }

            a_lexeme = variable{
                .m_identifier = l_identifier,
            };
        }
        // 5. quoted atom
        else if (is_quote(l_char))
        {
            std::string l_text;

            // Save the type of quotation. This allows us to match for closing quote.
            char l_quote_char = l_char;

            // The input was a quote character. Thus we should scan until the closing quote
            //     to produce a valid lexeme.
            while (
                // Consume char now
                a_istream.get(l_char) &&
                l_char != l_quote_char)
            {
                if (l_char == '\\')
                    escape(a_istream, l_char);

                l_text.push_back(l_char);
            }

            a_lexeme = atom{
                .m_text = l_text,
            };
        }
        // 6. unquoted atom
        else
        {
            std::string l_text;

            l_text.push_back(l_char);

            // The input was unquoted. Thus it should be treated as text,
            //     and we must terminate the text at a lexeme separator character OR
            //     when we reach EOF.

            // NOTE: Since we may simply hit EOF before any lexeme separator, we
            //     should not consume the EOF so as to prevent setting the failbit.
            while (
                // Chars we DO NOT want to consume
                !is_lex_stop_character(a_istream.peek()) &&
                // Get the char now
                a_istream.get(l_char))
            {
                l_text.push_back(l_char);
            }

            a_lexeme = atom{
                .m_text = l_text,
            };
        }

        return a_istream;
    }
}
