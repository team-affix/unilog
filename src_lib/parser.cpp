#include <iterator>
#include <algorithm>
#include <map>

#include "variant_functions.hpp"
#include "parser.hpp"
#include "lexer.hpp"

/// This macro function defines
///     getting a value from cache if key contained,
///     otherwise, computing value and caching it.
#define CACHE(cache, key, value) \
    (cache.contains(key) ? cache[key] : cache[key] = value)

namespace unilog
{

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

        if (std::holds_alternative<quoted_atom>(l_lexeme))
        {
            const quoted_atom &l_quoted_atom = std::get<quoted_atom>(l_lexeme);
            a_term_t = PL_new_term_ref();
            PL_put_atom_chars(a_term_t, l_quoted_atom.m_text.c_str());
        }
        else if (std::holds_alternative<unquoted_atom>(l_lexeme))
        {
            const unquoted_atom &l_unquoted_atom = std::get<unquoted_atom>(l_lexeme);
            a_term_t = PL_new_term_ref();
            PL_put_atom_chars(a_term_t, l_unquoted_atom.m_text.c_str());
        }
        else if (std::holds_alternative<variable>(l_lexeme))
        {
            const variable &l_variable = std::get<variable>(l_lexeme);
            a_term_t = l_variable.m_identifier == "_" ? PL_new_term_ref() : CACHE(a_var_alist, l_variable.m_identifier, PL_new_term_ref());
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

                // finally, extract the list_close
                lexeme l_list_close;
                a_istream >> l_list_close;

                if (!std::holds_alternative<list_close>(l_list_close))
                {
                    a_istream.setstate(std::ios::failbit);
                    return a_istream;
                }
            }
            else
            {
                // failed to extract a proper list terminator
                a_istream.setstate(std::ios::failbit);
                return a_istream;
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

    std::istream &operator>>(std::istream &a_istream, statement &a_statement)
    {
        // extract the command lexeme
        lexeme l_command;
        a_istream >> l_command;

        // lexing should result in an unquoted atom for the command
        if (!std::holds_alternative<unquoted_atom>(l_command))
        {
            a_istream.setstate(std::ios::failbit);
            return a_istream;
        }

        std::string l_command_text = std::get<unquoted_atom>(l_command).m_text;

        // declare the variable association-list
        std::map<std::string, term_t> l_var_alist;

        if (l_command_text == "axiom")
        {
            axiom_statement l_result;
            extract_term_t(a_istream, l_result.m_tag, l_var_alist);
            extract_term_t(a_istream, l_result.m_theorem, l_var_alist);
            a_statement = l_result;
        }
        else if (l_command_text == "guide")
        {
            guide_statement l_result;
            extract_term_t(a_istream, l_result.m_tag, l_var_alist);
            extract_term_t(a_istream, l_result.m_args, l_var_alist);
            extract_term_t(a_istream, l_result.m_guide, l_var_alist);
            a_statement = l_result;
        }
        else if (l_command_text == "infer")
        {
            infer_statement l_result;
            extract_term_t(a_istream, l_result.m_tag, l_var_alist);
            extract_term_t(a_istream, l_result.m_theorem, l_var_alist);
            extract_term_t(a_istream, l_result.m_guide, l_var_alist);
            a_statement = l_result;
        }
        else if (l_command_text == "refer")
        {
            refer_statement l_result;
            extract_term_t(a_istream, l_result.m_tag, l_var_alist);
            extract_term_t(a_istream, l_result.m_file_path, l_var_alist);
            a_statement = l_result;
        }
        else
        {
            a_istream.setstate(std::ios::failbit);
        }

        return a_istream;
    }

}
