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

static std::istream &consume_whitespace(std::istream &a_istream)
{
    char l_char;

    // Extract character (read until first non-whitespace)
    while (std::isspace(a_istream.peek()) != 0 && a_istream.get(l_char))
        ;

    return a_istream;
}

static std::istream &extract_unquoted_text(std::istream &a_istream, std::string &a_text)
{
    ////////////////////////////////////
    /////////// TEXT SECTION ///////////
    ////////////////////////////////////

    a_text.clear();

    char l_char;

    // unquoted lexemes may only contain alphanumeric chars and underscores.
    while (
        l_char = a_istream.peek(),
        // conditions for consumption
        (isalnum(l_char) || l_char == '_') &&
            // Consume char now
            a_istream.get(l_char))
    {
        a_text.push_back(l_char);
    }

    return a_istream;
}

static std::istream &extract_quoted_text(std::istream &a_istream, std::string &a_text)
{
    ////////////////////////////////////
    /////////// TEXT SECTION ///////////
    ////////////////////////////////////

    // save the type of quotation. then we can match for closing quote.
    int l_quote_char = a_istream.get();

    a_text.clear();

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

        a_text.push_back(l_char);
    }

    return a_istream;
}

namespace unilog
{

    bool operator==(const eol &a_lhs, const eol &a_rhs)
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

    bool operator==(const atom &a_lhs, const atom &a_rhs)
    {
        return a_lhs.m_text == a_rhs.m_text;
    }

    std::istream &operator>>(std::istream &a_istream, lexeme &a_lexeme)
    {
        // consume all leading whitespace
        consume_whitespace(a_istream);

        // get the char which indicates type of lexeme
        int l_indicator = a_istream.peek();

        if (!a_istream.good())
            return a_istream;

        if (l_indicator == ';')
        {
            a_istream.get(); // extract the character
            a_lexeme = eol{};
        }
        else if (l_indicator == '|')
        {
            a_istream.get(); // extract the character
            a_lexeme = list_separator{};
        }
        else if (l_indicator == '[')
        {
            a_istream.get(); // extract the character
            a_lexeme = list_open{};
        }
        else if (l_indicator == ']')
        {
            a_istream.get(); // extract the character
            a_lexeme = list_close{};
        }
        else if (isupper(l_indicator) || l_indicator == '_')
        {
            variable l_result;
            extract_unquoted_text(a_istream, l_result.m_identifier);
            a_lexeme = l_result;
        }
        else if (l_indicator == '\'' || l_indicator == '\"')
        {
            atom l_result;
            extract_quoted_text(a_istream, l_result.m_text);
            a_lexeme = l_result;
        }
        else if (isalpha(l_indicator)) // only lower-case letters
        {
            atom l_result;
            extract_unquoted_text(a_istream, l_result.m_text);
            a_lexeme = l_result;
        }
        else
        {
            a_istream.setstate(std::ios::failbit);
        }

        return a_istream;
    }

}
