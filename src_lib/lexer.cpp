#include <string>
#include <regex>
#include "lexer.hpp"

#define LEXEME_SEPARATOR ' '

#define LIST_OPEN_CHAR '('
#define LIST_CLOSE_CHAR ')'
#define QUOTE_CHAR '\''

namespace unilog
{
    int fxn()
    {
        return 15;
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
        char l_upper_hex_digit = a_istream.get();
        char l_lower_hex_digit = a_istream.get();
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
    std::istream &operator>>(std::istream &a_istream, lexeme &a_lexeme)
    {
        // Cases:
        // 1. list_open `(`
        // 2. list_close `)`
        // 3. text

        char l_char = a_istream.get();

        switch (l_char)
        {
        case LIST_OPEN_CHAR:
        {
            a_lexeme.m_token_type = token_types::list_open;
        }
        break;
        case LIST_CLOSE_CHAR:
        {
            a_lexeme.m_token_type = token_types::list_close;
        }
        break;
        case QUOTE_CHAR:
        {
            a_lexeme.m_token_type = token_types::quoted_text;

            // The input was a quote character. Thus we should scan until the closing quote
            //     to produce a valid lexeme.

            // Scan until closing quote.
            while (a_istream.get(l_char) && l_char != QUOTE_CHAR)
            {
                if (l_char == '\\')
                    escape(a_istream, l_char);

                a_lexeme.m_token_text.push_back(l_char);
            }
        }
        break;
        default:
        {
            a_lexeme.m_token_type = token_types::unquoted_text;

            // The input was unquoted. Thus it should be treated as text,
            //     and we must terminate the text at a lexeme separator character.
            while (a_istream.get(l_char) && l_char != LEXEME_SEPARATOR)
            {
                if (l_char == '\\')
                    escape(a_istream, l_char);

                a_lexeme.m_token_text.push_back(l_char);
            }
        }
        break;
        }

        return a_istream;
    }
}
