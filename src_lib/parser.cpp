#include <iterator>
#include <algorithm>

#include "variant_functions.hpp"
#include "parser.hpp"

#define AXIOM_COMMAND_KEYWORD "axiom"
#define GUIDE_COMMAND_KEYWORD "guide"
#define INFER_COMMAND_KEYWORD "infer"
#define REFER_COMMAND_KEYWORD "refer"

namespace unilog
{

    bool operator==(const term &a_lhs, const term &a_rhs)
    {
        return a_lhs.m_variant == a_rhs.m_variant;
    }

    bool operator==(const axiom_statement &a_lhs, const axiom_statement &a_rhs)
    {
        return a_lhs.m_tag == a_rhs.m_tag &&
               a_lhs.m_theorem == a_rhs.m_theorem;
    }

    bool operator==(const guide_statement &a_lhs, const guide_statement &a_rhs)
    {
        return a_lhs.m_tag == a_rhs.m_tag &&
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

    std::istream &extract_prolog_expression(std::istream &a_istream, term &a_term, int &a_list_depth)
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
            a_term.m_variant = std::get<variable>(l_lexeme);
        else if (std::holds_alternative<list_open>(l_lexeme))
        {
            std::list<term> l_list;

            // increment list depth BEFORE while loop.
            //     this is to avoid repeatedly incrementing each iteration
            ++a_list_depth;

            term l_prolog_subexpression;

            // extract until failure, this does NOT have to come from
            //     eof. It will come from a list_close lexeme or command as well.
            while (extract_prolog_expression(a_istream, l_prolog_subexpression, a_list_depth))
                l_list.push_back(l_prolog_subexpression);

            // reset the failbit flag for the stream
            a_istream.clear(a_istream.rdstate() & ~std::ios::failbit);

            a_term.m_variant = l_list;
        }
        else if (std::holds_alternative<list_close>(l_lexeme))
        {
            --a_list_depth;
            // indicate that iterative extraction should stop
            a_istream.setstate(std::ios::failbit);
        }
        else
        {
            // failed to extract prolog expression
            a_istream.setstate(std::ios::failbit);
            throw std::runtime_error("Failed to extract prolog expression: Expected prolog expression, found something else.");
        }

        return a_istream;
    }

    std::istream &operator>>(std::istream &a_istream, term &a_term)
    {
        // the paren stack
        int l_list_depth = 0;

        std::istream &l_result = extract_prolog_expression(a_istream, a_term, l_list_depth);

        if (l_list_depth > 0 || l_list_depth < 0)
            a_istream.setstate(std::ios::failbit); // open/close bracket count mismatch

        return l_result;
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
