#include <iterator>
#include <algorithm>

#include "variant_functions.hpp"
#include "parser.hpp"
#include "lexer.hpp"

#define AXIOM_COMMAND_KEYWORD "axiom"
#define GUIDE_COMMAND_KEYWORD "guide"
#define INFER_COMMAND_KEYWORD "infer"
#define REFER_COMMAND_KEYWORD "refer"

/// This macro function defines
///     getting a value from cache if key contained,
///     otherwise, computing value and caching it.
#define CACHE(cache, key, value) \
    (cache.contains(key) ? cache[key] : cache[key] = value)

namespace unilog
{

    bool operator==(const pl_term &a_lhs, const pl_term &a_rhs)
    {
        return PL_compare(a_lhs.native_handle(), a_rhs.native_handle()) == 0;
    }

    bool operator==(const axiom_statement &a_lhs, const axiom_statement &a_rhs)
    {
        return a_lhs.m_tag == a_rhs.m_tag &&
               a_lhs.m_theorem == a_rhs.m_theorem;
    }

    bool operator==(const guide_statement &a_lhs, const guide_statement &a_rhs)
    {
        return a_lhs.m_tag == a_rhs.m_tag &&
               a_lhs.m_args == a_rhs.m_args &&
               a_lhs.m_guide == a_rhs.m_guide;
    }

    bool operator==(const infer_statement &a_lhs, const infer_statement &a_rhs)
    {
        return a_lhs.m_tag == a_rhs.m_tag &&
               a_lhs.m_theorem == a_rhs.m_theorem &&
               a_lhs.m_guide == a_rhs.m_guide;
    }

    bool operator==(const refer_statement &a_lhs, const refer_statement &a_rhs)
    {
        return a_lhs.m_tag == a_rhs.m_tag &&
               a_lhs.m_file_path == a_rhs.m_file_path;
    }

    std::istream &extract_term_t(std::istream &a_istream, term_t &a_term_t, std::map<std::string, term_t> &a_var_alist)
    {
        lexeme l_lexeme;

        // Try to extract a lexeme.
        a_istream >> l_lexeme;

        // NOTE:
        //     here, we cannot check !good(), because EOFbit might be set due to peek()ing EOF
        //     in the lexer. Instead, check for failure to extract.
        if (a_istream.fail())
            return a_istream;

        if (std::holds_alternative<atom>(l_lexeme))
            a_term.m_variant = std::get<atom>(l_lexeme);
        else if (std::holds_alternative<variable>(l_lexeme))
        {
            std::string l_identifier = std::get<variable>(l_lexeme).m_identifier;
            a_term_t = CACHE(a_var_alist, l_identifier, PL_new_term_ref());
        }
        else if (std::holds_alternative<list_open>(l_lexeme))
        {
            std::list<term_t> l_list;

            term_t l_sub_term;

            // extract until failure, this does NOT have to come from
            //     eof. It will come from a list_close lexeme or command as well.
            while (extract_term_t(a_istream, l_sub_term, a_var_alist))
                l_list.push_back(l_sub_term);

            // reset the failbit flag for the stream
            a_istream.clear(a_istream.rdstate() & ~std::ios::failbit);

            // terminator will either be a list_close ']' or list_separator '|'
            lexeme l_list_terminator;
            a_istream >> l_list_terminator;

            // set the list's tail (into a_term_t)
            if (std::holds_alternative<list_close>(l_list_terminator))
            {
                PL_put_nil(a_term_t);
            }
            else if (std::holds_alternative<list_separator>(l_list_terminator))
            {
                // extract the tail of the list. could be var or atom, or any expr
                extract_term_t(a_istream, a_term_t, a_var_alist);
            }
            else
            {
                // failed to extract a proper list terminator
                a_istream.setstate(std::ios::failbit);
            }

            // reverse-iterate list
            for (auto l_it = l_list.rbegin(); l_it != l_list.rend(); l_it++)
            {
                // build list in reverse order
                PL_cons_list(a_term_t, *l_it, a_term_t);
            }
        }
        else
        {
            // failed to extract prolog expression
            a_istream.setstate(std::ios::failbit);
        }

        return a_istream;
    }

    std::istream &operator>>(std::istream &a_istream, axiom_statement &a_axiom_statement)
    {
        unquoted_atom l_command;
        a_istream >> l_command;

        if (l_command.m_text != AXIOM_COMMAND_KEYWORD)
        {
            a_istream.setstate(std::ios::failbit);
            return a_istream;
        }

        a_istream >> a_axiom_statement.m_tag;
        a_istream >> a_axiom_statement.m_theorem;

        return a_istream;
    }

    std::istream &operator>>(std::istream &a_istream, guide_statement &a_guide_statement)
    {
        unquoted_atom l_command;
        a_istream >> l_command;

        if (l_command.m_text != GUIDE_COMMAND_KEYWORD)
        {
            a_istream.setstate(std::ios::failbit);
            return a_istream;
        }

        a_istream >> a_guide_statement.m_tag;

        /////////// read the list open for args list
        list_open l_args_list_open;
        if (!(a_istream >> l_args_list_open))
            return a_istream;

        a_guide_statement.m_args.clear();

        /////////// read in list of args
        std::copy(std::istream_iterator<variable>(a_istream),
                  std::istream_iterator<variable>(),
                  std::back_inserter(a_guide_statement.m_args));

        // reset failbit flag
        a_istream.clear(a_istream.rdstate() & ~std::ios::failbit);

        /////////// read the list close for args list
        list_close l_args_list_close;
        if (!(a_istream >> l_args_list_close))
            return a_istream;

        a_istream >> a_guide_statement.m_guide;

        return a_istream;
    }

    std::istream &operator>>(std::istream &a_istream, infer_statement &a_infer_statement)
    {
        unquoted_atom l_command;
        a_istream >> l_command;

        if (l_command.m_text != INFER_COMMAND_KEYWORD)
        {
            a_istream.setstate(std::ios::failbit);
            return a_istream;
        }

        a_istream >> a_infer_statement.m_tag;
        a_istream >> a_infer_statement.m_theorem;
        a_istream >> a_infer_statement.m_guide;

        return a_istream;
    }

    std::istream &operator>>(std::istream &a_istream, refer_statement &a_refer_statement)
    {
        unquoted_atom l_command;
        a_istream >> l_command;

        if (l_command.m_text != REFER_COMMAND_KEYWORD)
        {
            a_istream.setstate(std::ios::failbit);
            return a_istream;
        }

        a_istream >> a_refer_statement.m_tag;
        a_istream >> a_refer_statement.m_file_path;

        return a_istream;
    }

}
