#ifndef LEXER_H
#define LEXER_H

#include <list>
#include <string>
#include <filesystem>
#include <iterator>

namespace unilog
{

    enum class token_types
    {
        list_open,     // `(`
        list_close,    // `)`
        quoted_text,   // `'abc'`, `'123'` etc.
        unquoted_text, // `axiom`, `infer`, `abc123`, `12345`, `+`, `'test123'` etc.
    };

    struct lexeme
    {
        token_types m_token_type;
        std::string m_token_text;
    };

    int fxn();

    std::istream &operator>>(std::istream &a_istream, lexeme &a_lexeme);

}

#endif
