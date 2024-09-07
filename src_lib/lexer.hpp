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
        list_open,  // `(`
        list_close, // `)`
        atom,       // `axiom`, `infer`, `abcDEF`, `12345`, `+`, `'test123'` etc.
        variable    // `A`, `Bcdef`, `A12abc`
    };

    struct lexeme
    {
        token_types m_token_type;
        std::string m_token_text;
    };

    bool operator==(const lexeme &a_first, const lexeme &a_second);

    std::istream &operator>>(std::istream &a_istream, lexeme &a_lexeme);

}

#endif
