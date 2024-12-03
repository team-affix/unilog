#ifndef EXECUTOR_HPP
#define EXECUTOR_HPP

#include "parser.hpp"

namespace unilog
{
    void execute(const axiom_statement &a_axiom_statement, term_t a_module_path);
    void execute(const redir_statement &a_redir_statement, term_t a_module_path);
    void execute(const infer_statement &a_infer_statement, term_t a_module_path);
    void execute(const refer_statement &a_refer_statement, term_t a_module_path);
}

int call_predicate(const std::string &a_functor, const std::vector<term_t> &a_args);
void wipe_database();

#endif
