#ifndef LEXER_H
#define LEXER_H

#include <list>
#include <string>
#include <filesystem>

namespace unilog
{

    enum class token_types
    {
        unknown,
        tag_term_separator, // `:`
        atom,               // `abc`
        list,               // `(abc 123)`
        end_of_line,        // `.`
    };

    struct lexeme
    {
        token_types m_token_type;
        std::string m_token;
    };

    std::list<lexeme> lex_file(const std::filesystem::path &a_path);
    std::list<lexeme> lex_string(const std::string &a_string);

}

#endif
