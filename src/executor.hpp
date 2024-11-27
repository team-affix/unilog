#ifndef EXECUTOR_HPP
#define EXECUTOR_HPP

#include "parser.hpp"

namespace unilog
{
    bool execute(const axiom_statement &a_axiom_statement, term_t a_module_path);
    bool execute(const redir_statement &a_redir_statement, term_t a_module_path);
    bool execute(const infer_statement &a_infer_statement, term_t a_module_path);
    bool execute(const refer_statement &a_refer_statement, term_t a_module_path);
}

#endif
