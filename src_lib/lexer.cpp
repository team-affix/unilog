#include <string>
#include <regex>
#include <stdexcept>
#include <cctype>
#include "lexer.hpp"

#define LIST_OPEN_CHAR '['
#define LIST_CLOSE_CHAR ']'
#define QUOTE_CHAR '\''
#define UNNAMED_VARIABLE_CHAR '_'

namespace unilog
{
    int fxn()
    {
        return 17;
    }
}

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
    bool operator==(const lexeme &a_first, const lexeme &a_second)
    {
        return a_first.m_token_type == a_second.m_token_type &&
               a_first.m_token_text == a_second.m_token_text;
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

        // Cases:
        // 1. list_open `(`
        // 2. list_close `)`
        // 3. text

        // Clear lexeme if prepopulated.
        a_lexeme = {};

        char l_char;

        // Extract character (read until first non-whitespace)
        while (a_istream.get(l_char) && std::isspace(l_char) != 0)
            ;

        // If reading the character failed, just early return.
        //     Extracting lexeme failed.
        if (!a_istream.good())
            return a_istream;

        switch (l_char)
        {
        case LIST_OPEN_CHAR:
        {
            a_lexeme.m_token_type = token_types::list_open;
            a_lexeme.m_token_text.push_back(l_char);
        }
        break;
        case LIST_CLOSE_CHAR:
        {
            a_lexeme.m_token_type = token_types::list_close;
            a_lexeme.m_token_text.push_back(l_char);
        }
        break;
        case QUOTE_CHAR:
        {
            a_lexeme.m_token_type = token_types::atom;

            // The input was a quote character. Thus we should scan until the closing quote
            //     to produce a valid lexeme.
            while (
                a_istream.get(l_char) &&
                l_char != QUOTE_CHAR)
            {
                if (l_char == '\\')
                    escape(a_istream, l_char);

                a_lexeme.m_token_text.push_back(l_char);
            }
        }
        break;
        default:
        {
            if (isupper(l_char) || l_char == UNNAMED_VARIABLE_CHAR)
                a_lexeme.m_token_type = token_types::variable;
            else
                a_lexeme.m_token_type = token_types::atom;

            a_lexeme.m_token_text.push_back(l_char);

            // The input was unquoted. Thus it should be treated as text,
            //     and we must terminate the text at a lexeme separator character OR
            //     when we reach EOF.

            // NOTE: Since we may simply hit EOF before any lexeme separator, we
            //     should not consume the EOF so as to prevent setting the failbit.
            while (
                // Chars we DO NOT want to consume
                a_istream.peek() != std::istream::traits_type::eof() &&
                a_istream.peek() != LIST_OPEN_CHAR &&
                a_istream.peek() != LIST_CLOSE_CHAR &&
                // Get the char now
                a_istream.get(l_char) &&
                std::isspace(l_char) == 0)
            {
                if (l_char == '\\')
                    escape(a_istream, l_char);

                a_lexeme.m_token_text.push_back(l_char);
            }
        }
        break;
        }

        if (a_istream.fail())
        {
            throw std::runtime_error("Failed to lex lexeme.");
        }

        return a_istream;
    }
}
