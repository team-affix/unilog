#include <string>
#include "lexer.hpp"

#define LEXEME_SEPARATOR ' '

#define LIST_OPEN_CHAR '('
#define LIST_CLOSE_CHAR ')'
#define QUOTE_CHAR '\''

int unilog::fxn()
{
    return 14;
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
                a_lexeme.m_token_text.push_back(l_char);
        }
        break;
        default:
        {
            a_lexeme.m_token_type = token_types::unquoted_text;

            // The input was unquoted. Thus it should be treated as text,
            //     and we must terminate the text at a lexeme separator character.
            while (a_istream.get(l_char) && l_char != LEXEME_SEPARATOR)
                a_lexeme.m_token_text.push_back(l_char);
        }
        break;
        }

        return a_istream;
    }
}
