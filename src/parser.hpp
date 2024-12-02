#ifndef PARSER_HPP
#define PARSER_HPP

#include <memory>
#include <variant>
#include <istream>
#include <string>
#include <map>
#include <list>
#include <SWI-Prolog.h>

#define ERR_MSG_UNIFY "Error: failed to unify terms"
#define ERR_MSG_CONS_LIST "Error: failed to cons list"
#define ERR_MSG_GET_ATOM_CHARS "Error: failed to get atom chars"
#define ERR_MSG_PUT_ATOM_CHARS "Error: failed to put atom chars"
#define ERR_MSG_PUT_NIL "Error: failed to put nil"

#define ERR_MSG_NO_LIST_CLOSE "Error: no list close"
#define ERR_MSG_MALFORMED_TERM "Error: malformed term"
#define ERR_MSG_INVALID_COMMAND "Error: invalid command"
#define ERR_MSG_MALFORMED_STMT "Error: malformed statement"
#define ERR_MSG_NO_EOL "Error: expected end-of-line (;)"

namespace unilog
{

    struct axiom_statement
    {
        term_t m_tag;
        term_t m_theorem;
    };

    struct redir_statement
    {
        term_t m_tag;
        term_t m_guide;
    };

    struct infer_statement
    {
        term_t m_tag;
        term_t m_guide;
    };

    struct refer_statement
    {
        term_t m_tag;
        term_t m_file_path;
    };

    bool operator==(const axiom_statement &a_lhs, const axiom_statement &a_rhs);
    bool operator==(const redir_statement &a_lhs, const redir_statement &a_rhs);
    bool operator==(const infer_statement &a_lhs, const infer_statement &a_rhs);
    bool operator==(const refer_statement &a_lhs, const refer_statement &a_rhs);

    using statement = std::variant<
        axiom_statement,
        redir_statement,
        infer_statement,
        refer_statement>;

    std::istream &operator>>(std::istream &a_istream, statement &a_statement);

}

// determines if the two terms share the same form. consult function definition for more details.
// this WILL MODIFY these terms, on the current prolog frame.
bool equal_forms(term_t a_lhs, term_t a_rhs);
term_t make_nil();
term_t make_atom(const std::string &a_text);
term_t make_list(const std::list<term_t> &a_elements, term_t a_tail = make_nil());
term_t make_var(const std::string &a_identifier, std::map<std::string, term_t> &a_var_alist);

#endif
