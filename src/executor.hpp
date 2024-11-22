#ifndef EXECUTOR_HPP
#define EXECUTOR_HPP

#include "parser.hpp"

namespace unilog
{
    bool execute(const axiom_statement &a_axiom_statement);
    bool execute(const guide_statement &a_guide_statement);
    bool execute(const infer_statement &a_infer_statement);
    bool execute(const refer_statement &a_refer_statement);
}

#endif
