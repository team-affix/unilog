#include <iterator>
#include <algorithm>

#include "parser.hpp"

namespace unilog
{
    bool operator==(const prolog_expression &a_lhs, const prolog_expression &a_rhs)
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

    std::istream &operator>>(std::istream &a_istream, prolog_expression &a_prolog_expression)
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
            a_prolog_expression.m_variant = std::get<atom>(l_lexeme);
        else if (std::holds_alternative<variable>(l_lexeme))
            a_prolog_expression.m_variant = std::get<variable>(l_lexeme);
        else if (std::holds_alternative<list_open>(l_lexeme))
        {
            std::list<prolog_expression> l_list;

            // extract until failure, this does NOT have to come from
            //     eof. It will come from a list_close lexeme or command as well.
            std::copy(
                std::istream_iterator<prolog_expression>(a_istream),
                std::istream_iterator<prolog_expression>(),
                std::back_inserter(l_list));

            if (a_istream.eof())
                // we do expect the failbit to be set, but NOT EOF.
                //     if EOF is set, that means we did not find a closing brace.
                throw std::runtime_error("Failed to find closing bracket when extracting list, reached EOF.");

            // reset the state flags for the stream
            a_istream.clear();

            a_prolog_expression.m_variant = l_list;
        }
        else if (std::holds_alternative<list_close>(l_lexeme))
        {
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

    std::istream &operator>>(std::istream &a_istream, statement &a_statement)
    {
        return a_istream;
    }
}
