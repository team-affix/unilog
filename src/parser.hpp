#ifndef PARSER_H
#define PARSER_H

#include <map>
#include <string>
#include <variant>
#include <filesystem>
#include <SWI-Prolog.h>

namespace unilog
{

    struct refer_decl
    {
        std::filesystem::path m_file_path;
    };

    struct guide_decl
    {
        term_t m_guide;
    };

    struct axiom_decl
    {
        term_t m_theorem;
    };

    struct infer_decl
    {
        term_t m_theorem;
        term_t m_guide;
    };

    using command_decl = std::variant<refer_decl, guide_decl, axiom_decl, infer_decl>;

    std::map<term_t, command_decl> parse_file(const std::filesystem::path &a_path);
    std::map<term_t, command_decl> parse_string(const std::string &a_string);

}

#endif
