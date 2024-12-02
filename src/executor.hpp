#ifndef EXECUTOR_HPP
#define EXECUTOR_HPP

#include "parser.hpp"

#define ERR_MSG_NOT_A_FILE "Error: not a file"
#define ERR_MSG_FILE_OPEN "Error: file failed to open"
#define ERR_MSG_DECL_THEOREM "Error: failed to declare theorem"
#define ERR_MSG_DECL_REDIR "Error: failed to declare redirect"
#define ERR_MSG_INFER "Error: inference failed"

namespace unilog
{
    void execute(const axiom_statement &a_axiom_statement, term_t a_module_path);
    void execute(const redir_statement &a_redir_statement, term_t a_module_path);
    void execute(const infer_statement &a_infer_statement, term_t a_module_path);
    void execute(const refer_statement &a_refer_statement, term_t a_module_path);
}

void wipe_database();

#endif
