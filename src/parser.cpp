#include <iterator>
#include <algorithm>
#include <iostream>
#include <random>

#include "parser.hpp"
#include "lexer.hpp"

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

/// This macro function defines
///     getting a value from cache if key contained,
///     otherwise, computing value and caching it.
#define CACHE(cache, key, value) \
    (cache.contains(key) ? cache[key] : cache[key] = value)

namespace unilog
{

    static std::istream &extract_term_t(std::istream &a_istream, std::map<std::string, term_t> &a_var_alist, term_t a_term_t, bool *a_list_terminated = nullptr)
    {
        // we assume a_term_t is already assigned to PL_new_term_ref()

        lexeme l_lexeme;

        // Try to extract a lexeme.
        a_istream >> l_lexeme;

        // NOTE:
        //     here, we cannot check !good(), because EOFbit might be set due to peek()ing EOF
        //     in the lexer. Instead, check for failure to extract.
        if (a_istream.fail())
            return a_istream;

        if (std::holds_alternative<atom>(l_lexeme))
        {
            const atom &l_atom = std::get<atom>(l_lexeme);

            /////////////////////////////////////////
            // try to bind the term to an atom with specific text
            /////////////////////////////////////////
            if (!PL_put_atom_chars(a_term_t, l_atom.m_text.c_str()))
                throw std::runtime_error(ERR_MSG_PUT_ATOM_CHARS);
        }
        else if (std::holds_alternative<variable>(l_lexeme))
        {
            const variable &l_variable = std::get<variable>(l_lexeme);

            // singletons never get entries in the table
            if (l_variable.m_identifier == "_")
                return a_istream;

            /////////////////////////////////////////
            // see if this variable already has entry
            /////////////////////////////////////////
            auto l_alist_entry = a_var_alist.find(l_variable.m_identifier);

            /////////////////////////////////////////
            // no entry exists, create one
            /////////////////////////////////////////
            if (l_alist_entry == a_var_alist.end())
                l_alist_entry = a_var_alist.insert({l_variable.m_identifier, PL_new_term_ref()}).first;

            /////////////////////////////////////////
            // unify current term with entry
            /////////////////////////////////////////
            if (!PL_unify(a_term_t, l_alist_entry->second))
                throw std::runtime_error(ERR_MSG_UNIFY);
        }
        else if (std::holds_alternative<list_open>(l_lexeme))
        {
            std::list<term_t> l_list;

            // initialize flag for list termination
            bool l_list_terminated = false;

            // extract until failure, this does NOT have to come from
            //     eof. It will come from any list termination process.
            //     we supply a_term_t as the list tail reference, because
            //     we will build the list in reverse order, starting from the tail.
            while (!l_list_terminated)
            {
                term_t l_sub_term = PL_new_term_ref();

                /////////////////////////////////////////
                // extract one subterm. final subterm will be the list's tail
                /////////////////////////////////////////
                extract_term_t(a_istream, a_var_alist, l_sub_term, &l_list_terminated);

                if (a_istream.fail())
                    throw std::runtime_error(ERR_MSG_NO_LIST_CLOSE);

                l_list.push_back(l_sub_term);
            }

            /////////////////////////////////////////
            // extract the tail of the list
            /////////////////////////////////////////
            if (!PL_unify(a_term_t, l_list.back()))
                throw std::runtime_error(ERR_MSG_UNIFY);
            l_list.pop_back();

            /////////////////////////////////////////
            // build list in reverse order
            /////////////////////////////////////////
            for (auto l_it = l_list.rbegin(); l_it != l_list.rend(); l_it++)
            {
                if (!PL_cons_list(a_term_t, *l_it, a_term_t))
                    throw std::runtime_error(ERR_MSG_CONS_LIST);
            }
        }
        else if (std::holds_alternative<list_close>(l_lexeme))
        {
            // this basically checks if the list termination was expected
            if (a_list_terminated == nullptr)
                throw std::runtime_error(ERR_MSG_MALFORMED_TERM);

            /////////////////////////////////////////
            // default tail is always nil
            /////////////////////////////////////////
            if (!PL_put_nil(a_term_t))
                throw std::runtime_error(ERR_MSG_PUT_NIL);

            /////////////////////////////////////////
            // notify the caller the list terminated
            /////////////////////////////////////////
            *a_list_terminated = true;
        }
        else if (std::holds_alternative<list_separator>(l_lexeme))
        {
            // this basically checks if the list termination was expected
            if (a_list_terminated == nullptr)
                throw std::runtime_error(ERR_MSG_MALFORMED_TERM);

            /////////////////////////////////////////
            // extract the tail of the list.
            // could be var or atom, or any expr
            /////////////////////////////////////////
            extract_term_t(a_istream, a_var_alist, a_term_t);

            /////////////////////////////////////////
            // finally, extract the list_close
            /////////////////////////////////////////
            lexeme l_list_close;
            a_istream >> l_list_close;

            if (a_istream.fail() || !std::holds_alternative<list_close>(l_list_close))
                throw std::runtime_error(ERR_MSG_NO_LIST_CLOSE);

            /////////////////////////////////////////
            // notify the caller the list terminated
            /////////////////////////////////////////
            *a_list_terminated = true;
        }
        else
        {
            // failed to extract prolog expression
            throw std::runtime_error(ERR_MSG_MALFORMED_TERM);
        }

        return a_istream;
    }

    bool operator==(const axiom_statement &a_lhs, const axiom_statement &a_rhs)
    {
        fid_t l_frame = PL_open_foreign_frame();

        bool l_result = equal_forms(a_lhs.m_tag, a_rhs.m_tag) &&
                        equal_forms(a_lhs.m_theorem, a_rhs.m_theorem);

        PL_discard_foreign_frame(l_frame);

        return l_result;
    }

    bool operator==(const redir_statement &a_lhs, const redir_statement &a_rhs)
    {
        fid_t l_frame = PL_open_foreign_frame();

        bool l_result = equal_forms(a_lhs.m_tag, a_rhs.m_tag) &&
                        equal_forms(a_lhs.m_guide, a_rhs.m_guide);

        PL_discard_foreign_frame(l_frame);

        return l_result;
    }

    bool operator==(const infer_statement &a_lhs, const infer_statement &a_rhs)
    {
        fid_t l_frame = PL_open_foreign_frame();

        bool l_result = equal_forms(a_lhs.m_tag, a_rhs.m_tag) &&
                        equal_forms(a_lhs.m_guide, a_rhs.m_guide);

        PL_discard_foreign_frame(l_frame);

        return l_result;
    }

    bool operator==(const refer_statement &a_lhs, const refer_statement &a_rhs)
    {
        fid_t l_frame = PL_open_foreign_frame();

        bool l_result = equal_forms(a_lhs.m_tag, a_rhs.m_tag) &&
                        equal_forms(a_lhs.m_file_path, a_rhs.m_file_path);

        PL_discard_foreign_frame(l_frame);

        return l_result;
    }

    std::istream &operator>>(std::istream &a_istream, statement &a_statement)
    {
        /////////////////////////////////////////
        // extract the command lexeme
        /////////////////////////////////////////
        lexeme l_command;
        a_istream >> l_command;

        // atom is expected for the command
        if (a_istream.fail() || !std::holds_alternative<atom>(l_command))
            throw std::runtime_error(ERR_MSG_MALFORMED_STMT);

        std::string l_command_text = std::get<atom>(l_command).m_text;

        /////////////////////////////////////////
        // declare the variable association-list
        /////////////////////////////////////////
        std::map<std::string, term_t> l_var_alist;

        if (l_command_text == "axiom")
        {
            axiom_statement l_result;

            /////////////////////////////////////////
            // creates new term refs
            /////////////////////////////////////////
            l_result.m_tag = PL_new_term_ref();
            l_result.m_theorem = PL_new_term_ref();

            if (!(extract_term_t(a_istream, l_var_alist, l_result.m_tag) &&
                  extract_term_t(a_istream, l_var_alist, l_result.m_theorem)))
                throw std::runtime_error(ERR_MSG_MALFORMED_STMT);

            a_statement = l_result;
        }
        else if (l_command_text == "redir")
        {
            redir_statement l_result;

            /////////////////////////////////////////
            // creates new term refs
            /////////////////////////////////////////
            l_result.m_tag = PL_new_term_ref();
            l_result.m_guide = PL_new_term_ref();

            if (!(extract_term_t(a_istream, l_var_alist, l_result.m_tag) &&
                  extract_term_t(a_istream, l_var_alist, l_result.m_guide)))
                throw std::runtime_error(ERR_MSG_MALFORMED_STMT);

            a_statement = l_result;
        }
        else if (l_command_text == "infer")
        {
            infer_statement l_result;

            /////////////////////////////////////////
            // creates new term refs
            /////////////////////////////////////////
            l_result.m_tag = PL_new_term_ref();
            l_result.m_guide = PL_new_term_ref();

            if (!(extract_term_t(a_istream, l_var_alist, l_result.m_tag) &&
                  extract_term_t(a_istream, l_var_alist, l_result.m_guide)))
                throw std::runtime_error(ERR_MSG_MALFORMED_STMT);

            a_statement = l_result;
        }
        else if (l_command_text == "refer")
        {
            refer_statement l_result;

            /////////////////////////////////////////
            // creates new term refs
            /////////////////////////////////////////
            l_result.m_tag = PL_new_term_ref();
            l_result.m_file_path = PL_new_term_ref();

            if (!(extract_term_t(a_istream, l_var_alist, l_result.m_tag) &&
                  extract_term_t(a_istream, l_var_alist, l_result.m_file_path)))
                throw std::runtime_error(ERR_MSG_MALFORMED_STMT);

            a_statement = l_result;
        }
        else
        {
            throw std::runtime_error(ERR_MSG_INVALID_COMMAND);
        }

        /////////////////////////////////////////
        // extracts the expected eol character.
        /////////////////////////////////////////
        lexeme l_eol;
        a_istream >> l_eol;

        if (a_istream.fail() || !std::holds_alternative<eol>(l_eol))
            throw std::runtime_error(ERR_MSG_NO_EOL);

        return a_istream;
    }

}

////////////////////////////////
//// HELPERS
////////////////////////////////

term_t make_nil()
{
    term_t l_result = PL_new_term_ref();
    PL_put_nil(l_result);
    return l_result;
}

term_t make_atom(const std::string &a_text)
{
    term_t l_result = PL_new_term_ref();
    PL_put_atom_chars(l_result, a_text.c_str());
    return l_result;
}

term_t make_list(const std::list<term_t> &a_elements, term_t a_tail)
{
    term_t l_result = PL_new_term_ref();

    if (!PL_unify(l_result, a_tail))
        throw std::runtime_error("Error: failed to unify terms.");

    for (auto l_it = a_elements.rbegin(); l_it != a_elements.rend(); l_it++)
    {
        // just toss it into an assertion, since we want it to always succeed.
        if (!PL_cons_list(l_result, *l_it, l_result))
            throw std::runtime_error("Error: failed to unify terms.");
    }

    return l_result;
}

term_t make_var(const std::string &a_identifier, std::map<std::string, term_t> &a_var_alist)
{
    term_t l_result = PL_new_term_ref();

    // singletons never get entries in the table
    if (a_identifier == "_")
        return l_result;

    /////////////////////////////////////////
    // see if this variable already has entry
    /////////////////////////////////////////
    auto l_entry = a_var_alist.find(a_identifier);

    /////////////////////////////////////////
    // no entry exists
    /////////////////////////////////////////
    if (l_entry == a_var_alist.end())
        l_entry = a_var_alist.insert({a_identifier, l_result}).first;

    /////////////////////////////////////////
    // entry already exists;
    /////////////////////////////////////////
    if (!PL_unify(l_result, l_entry->second))
        throw std::runtime_error("Error: failed to unify terms.");

    return l_result;
}

static std::string random_string(size_t a_length)
{
    // Character set: alphanumeric (you can customize this)
    static const std::string s_chars =
        "abcdefghijklmnopqrstuvwxyz"
        "ABCDEFGHIJKLMNOPQRSTUVWXYZ"
        "0123456789";

    // Random number generator
    static std::random_device s_rd;          // Seed
    static std::mt19937 s_generator(s_rd()); // Random-number engine
    static std::uniform_int_distribution<> s_distribution(0, s_chars.size() - 1);

    // Generate random string
    std::string l_result;

    for (size_t i = 0; i < a_length; ++i)
    {
        l_result += s_chars[s_distribution(s_generator)];
    }

    return l_result;
}

bool equal_forms(term_t a_lhs, term_t a_rhs)
{
    // Formal equivalence is different than ability to unify,
    //     and is different than PL_compare() == 0.
    //
    // Examples of formal equivalence:
    // 1.
    //     [X X Y]
    //     [A A B]
    // 2.
    //     [X [X a] b]
    //     [A [A a] b]
    //
    // Examples of formal inequivalence:
    // 1.
    //     [X X Y]
    //     [A A A]
    // 2.
    //     [a a a]
    //     [A A A]
    //

    if (PL_get_nil(a_lhs) && PL_get_nil(a_rhs))
    {
        /////////////////////////////////////////
        // nil is universal
        /////////////////////////////////////////
        return true;
    }

    if (PL_is_atom(a_lhs) && PL_is_atom(a_rhs))
    {
        /////////////////////////////////////////
        // simply compare the atoms
        /////////////////////////////////////////
        return PL_compare(a_lhs, a_rhs) == 0;
    }

    if (PL_is_list(a_lhs) && PL_is_list(a_rhs))
    {
        term_t l_lhs_car = PL_new_term_ref();
        term_t l_lhs_cdr = PL_new_term_ref();
        if (!PL_get_list(a_lhs, l_lhs_car, l_lhs_cdr))
            return false;

        term_t l_rhs_car = PL_new_term_ref();
        term_t l_rhs_cdr = PL_new_term_ref();
        if (!PL_get_list(a_rhs, l_rhs_car, l_rhs_cdr))
            return false;

        /////////////////////////////////////////
        // ensure both the cars and cdrs are formally equivalent
        /////////////////////////////////////////
        return equal_forms(l_lhs_car, l_rhs_car) &&
               equal_forms(l_lhs_cdr, l_rhs_cdr);
    }

    if (PL_is_variable(a_lhs) && PL_is_variable(a_rhs))
    {
        constexpr int VAR_RANDOM_BIND_LEN = 50;

        /////////////////////////////////////////
        // generate a random binding string.
        // the purpose of this is the ensure that
        // all instances of this variable assume this new value,
        // which will reveal the difference in distribution
        // of the lhs variable and rhs variable.
        /////////////////////////////////////////
        std::string l_random_string = random_string(VAR_RANDOM_BIND_LEN);

        /////////////////////////////////////////
        // construct random atom
        /////////////////////////////////////////
        term_t l_random_atom = PL_new_term_ref();
        if (!PL_put_atom_chars(l_random_atom, l_random_string.c_str()))
            return false;

        /////////////////////////////////////////
        // unify random atom into both vars
        /////////////////////////////////////////
        if (!PL_unify(l_random_atom, a_lhs))
            return false;
        if (!PL_unify(l_random_atom, a_rhs))
            return false;

        return true;
    }

    return false;
}

#ifdef UNIT_TEST

#include <fstream>
#include <iterator>
#include "test_utils.hpp"

////////////////////////////////
//// TESTS
////////////////////////////////

static void test_make_nil()
{
    fid_t l_frame_id = PL_open_foreign_frame();

    term_t l_nil = make_nil();

    assert(PL_get_nil(l_nil));

    PL_discard_foreign_frame(l_frame_id);
}

static void test_make_atom()
{
    fid_t l_frame_id = PL_open_foreign_frame();

    // ensure distinctness from nil
    {
        term_t l_atom = make_atom("abc");
        assert(PL_compare(l_atom, make_nil()) != 0);
    };

    // test atom contraction
    {
        term_t l_atom_0 = make_atom("abc");
        term_t l_atom_1 = make_atom("abc");
        assert(PL_compare(l_atom_0, l_atom_1) == 0);
    };

    // test atom contraction
    {
        term_t l_atom_0 = make_atom("");
        term_t l_atom_1 = make_atom("");
        assert(PL_compare(l_atom_0, l_atom_1) == 0);
    };

    // test different atoms don't contract
    {
        term_t l_atom_0 = make_atom("abc1");
        term_t l_atom_1 = make_atom("abc2");
        assert(PL_compare(l_atom_0, l_atom_1) != 0);
    };

    // test different atoms don't contract
    {
        term_t l_atom_0 = make_atom("");
        term_t l_atom_1 = make_atom("abc2");
        assert(PL_compare(l_atom_0, l_atom_1) != 0);
    };

    PL_discard_foreign_frame(l_frame_id);
}

static void test_make_list()
{
    fid_t l_frame_id = PL_open_foreign_frame();

    // test make empty list
    {
        assert(PL_get_nil(make_list({})));
    };

    // 1-element list
    {
        term_t l_car = make_atom("a");
        term_t l_native_list = PL_new_term_ref();
        assert(PL_cons_list(l_native_list, l_car, make_nil()));
        assert(PL_compare(make_list({l_car}), l_native_list) == 0);
    };

    // pair of atoms (car/cdr)
    {
        term_t l_car = make_atom("a");
        term_t l_cdr = make_atom("b");
        term_t l_native_list = PL_new_term_ref();
        assert(PL_cons_list(l_native_list, l_car, l_cdr));
        assert(PL_compare(make_list({l_car}, l_cdr), l_native_list) == 0);
    };

    // 2-element list is NOT a pair (car/cdr of atoms).
    {
        term_t l_car = make_atom("a");
        term_t l_cdr = make_atom("b");
        term_t l_native_list = PL_new_term_ref();
        assert(PL_cons_list(l_native_list, l_car, l_cdr));
        assert(PL_compare(make_list({l_car, l_cdr}), l_native_list) != 0);
    };

    // 2-element list is NOT a pair (car/cdr of atoms).
    {
        term_t l_car = make_atom("a");
        term_t l_cdr = make_atom("b");
        term_t l_native_list = PL_new_term_ref();
        assert(PL_cons_list(l_native_list, l_car, l_cdr));
        assert(PL_compare(make_list({l_car, l_cdr}), l_native_list) != 0);
    };

    // 2-element list
    {
        term_t l_car = make_atom("a");
        term_t l_cadr = make_atom("b");

        term_t l_native_list = PL_new_term_ref();
        term_t l_cdr = PL_new_term_ref();

        assert(PL_cons_list(l_cdr, l_cadr, make_nil()));
        assert(PL_cons_list(l_native_list, l_car, l_cdr));

        assert(PL_compare(make_list({l_car, l_cadr}), l_native_list) == 0);
    };

    // 2-element list with non-nil cddr
    {
        term_t l_car = make_atom("a");
        term_t l_cadr = make_atom("b");

        term_t l_native_list = PL_new_term_ref();
        term_t l_cdr = PL_new_term_ref();
        term_t l_cddr = make_atom("cddr");

        assert(PL_cons_list(l_cdr, l_cadr, l_cddr));
        assert(PL_cons_list(l_native_list, l_car, l_cdr));

        assert(PL_compare(make_list({l_car, l_cadr}, l_cddr), l_native_list) == 0);
    };

    // 3-element list
    {
        term_t l_car = make_atom("a");
        term_t l_cadr = make_atom("b");
        term_t l_caddr = make_atom("c");

        term_t l_native_list = PL_new_term_ref();
        term_t l_cdr = PL_new_term_ref();
        term_t l_cddr = PL_new_term_ref();

        assert(PL_cons_list(l_cddr, l_caddr, make_nil()));
        assert(PL_cons_list(l_cdr, l_cadr, l_cddr));
        assert(PL_cons_list(l_native_list, l_car, l_cdr));

        assert(PL_compare(make_list({l_car, l_cadr, l_caddr}), l_native_list) == 0);
    };

    // make sure cdr gets effectively treated as the "rest" of a list
    {
        term_t l_cdr = make_list({make_atom("b")});
        term_t l_car = make_atom("a");

        term_t l_native_list = PL_new_term_ref();
        assert(PL_cons_list(l_native_list, l_car, l_cdr));

        assert(PL_compare(make_list({make_atom("a"), make_atom("b")}), l_native_list) == 0);
    };

    // nested list [[abc] d e]
    {
        term_t l_car = PL_new_term_ref();
        term_t l_caar = make_atom("abc");

        assert(PL_cons_list(l_car, l_caar, make_nil()));

        term_t l_cadr = make_atom("d");
        term_t l_caddr = make_atom("e");

        term_t l_native_list = PL_new_term_ref();
        term_t l_cdr = PL_new_term_ref();
        term_t l_cddr = PL_new_term_ref();

        assert(PL_cons_list(l_cddr, l_caddr, make_nil()));
        assert(PL_cons_list(l_cdr, l_cadr, l_cddr));
        assert(PL_cons_list(l_native_list, l_car, l_cdr));

        assert(PL_compare(make_list({make_list({l_caar}), l_cadr, l_caddr}), l_native_list) == 0);
    };

    PL_discard_foreign_frame(l_frame_id);
}

static void test_make_variable()
{
    fid_t l_frame_id = PL_open_foreign_frame();

    // distinct singletons
    {
        std::map<std::string, term_t> l_var_alist;
        term_t l_var_0 = make_var("_", l_var_alist);
        term_t l_var_1 = make_var("_", l_var_alist);
        assert(PL_compare(l_var_0, l_var_1) != 0);
        assert(!l_var_alist.contains("_")); // make sure singletons never get inserted
    };

    // distinct singleton, named
    {
        std::map<std::string, term_t> l_var_alist;
        term_t l_var_0 = make_var("_", l_var_alist);
        term_t l_var_1 = make_var("A", l_var_alist);
        assert(PL_compare(l_var_0, l_var_1) != 0);
        assert(!l_var_alist.contains("_")); // make sure singletons never get inserted
        assert(l_var_alist.contains("A"));  // named vars should be keys
    };

    // same-name vars contract
    {
        std::map<std::string, term_t> l_var_alist;
        term_t l_var_0 = make_var("A", l_var_alist);
        term_t l_var_1 = make_var("A", l_var_alist);
        assert(PL_compare(l_var_0, l_var_1) == 0);
        assert(l_var_alist.contains("A")); // named vars should be keys
        assert(l_var_alist.size() == 1);
    };

    // eq and inequal vars
    {
        std::map<std::string, term_t> l_var_alist;
        term_t l_var_0 = make_var("A", l_var_alist);
        term_t l_var_1 = make_var("B", l_var_alist);
        term_t l_var_2 = make_var("A", l_var_alist);
        assert(PL_compare(l_var_0, l_var_1) != 0);
        assert(PL_compare(l_var_0, l_var_2) == 0);
        assert(l_var_alist.contains("A"));
        assert(l_var_alist.contains("B"));
        assert(l_var_alist.size() == 2);
    };

    PL_discard_foreign_frame(l_frame_id);
}

static void test_random_string()
{
    // check outputs different value each time
    {
        std::string l_lhs = random_string(30);
        std::string l_rhs = random_string(30);
        assert(l_lhs != l_rhs);
    };

    // check size is correct
    {
        constexpr size_t STR_SZ = 30;
        std::string l_str = random_string(STR_SZ);
        assert(l_str.size() == STR_SZ);
    };

    // check size is correct
    {
        constexpr size_t STR_SZ = 10;
        std::string l_str = random_string(STR_SZ);
        assert(l_str.size() == STR_SZ);
    };

    // check size is correct
    {
        constexpr size_t STR_SZ = 1;
        std::string l_str = random_string(STR_SZ);
        assert(l_str.size() == STR_SZ);
    };

    // check size is correct
    {
        constexpr size_t STR_SZ = 12;
        std::string l_str = random_string(STR_SZ);
        assert(l_str.size() == STR_SZ);
    };
}

static void test_equal_forms()
{
    // nil == nil
    {
        fid_t l_frame = PL_open_foreign_frame();

        std::map<std::string, term_t> l_var_alist;

        term_t l_lhs = make_list({});
        term_t l_rhs = make_list({});

        assert(equal_forms(l_lhs, l_rhs));

        PL_discard_foreign_frame(l_frame);
    };

    // equal atom check
    {
        fid_t l_frame = PL_open_foreign_frame();

        term_t l_lhs = make_atom("abc");
        term_t l_rhs = make_atom("abc");

        assert(equal_forms(l_lhs, l_rhs));

        PL_discard_foreign_frame(l_frame);
    };

    // inequal atom check
    {
        fid_t l_frame = PL_open_foreign_frame();

        term_t l_lhs = make_atom("abc");
        term_t l_rhs = make_atom("abc1");

        assert(!equal_forms(l_lhs, l_rhs));

        PL_discard_foreign_frame(l_frame);
    };

    {
        fid_t l_frame = PL_open_foreign_frame();

        std::map<std::string, term_t> l_var_alist;

        term_t l_lhs = make_atom("abc");
        term_t l_rhs = make_var("A", l_var_alist);

        assert(PL_is_variable(l_rhs));
        assert(!equal_forms(l_lhs, l_rhs));
        assert(PL_is_variable(l_rhs)); // rhs REMAINS var since lhs was not var

        PL_discard_foreign_frame(l_frame);
    };

    // singular vars have eq forms
    {
        fid_t l_frame = PL_open_foreign_frame();

        std::map<std::string, term_t> l_var_alist;

        term_t l_lhs = make_var("A", l_var_alist);
        term_t l_rhs = make_var("B", l_var_alist);

        assert(PL_is_variable(l_lhs));
        assert(PL_is_variable(l_rhs));
        assert(equal_forms(l_lhs, l_rhs));
        assert(!PL_is_variable(l_lhs)); // vars will be bound to random atoms
        assert(!PL_is_variable(l_rhs)); // vars will be bound to random atoms

        PL_discard_foreign_frame(l_frame);
    };

    // single-element list eq
    {
        fid_t l_frame = PL_open_foreign_frame();

        std::map<std::string, term_t> l_var_alist;

        term_t l_lhs = make_list({make_atom("a")});
        term_t l_rhs = make_list({make_atom("a")});

        assert(equal_forms(l_lhs, l_rhs));

        PL_discard_foreign_frame(l_frame);
    };

    // single-element list neq
    {
        fid_t l_frame = PL_open_foreign_frame();

        std::map<std::string, term_t> l_var_alist;

        term_t l_lhs = make_list({make_atom("a")});
        term_t l_rhs = make_list({make_atom("b")});

        assert(!equal_forms(l_lhs, l_rhs));

        PL_discard_foreign_frame(l_frame);
    };

    // single-element list neq
    {
        fid_t l_frame = PL_open_foreign_frame();

        std::map<std::string, term_t> l_var_alist;

        term_t l_lhs = make_list({make_atom("a")});
        term_t l_rhs = make_list({make_var("A", l_var_alist)});

        assert(!equal_forms(l_lhs, l_rhs));

        PL_discard_foreign_frame(l_frame);
    };

    // car & cdr eq
    {
        fid_t l_frame = PL_open_foreign_frame();

        std::map<std::string, term_t> l_var_alist;

        term_t l_lhs = make_list({make_atom("a")}, make_atom("b"));
        term_t l_rhs = make_list({make_atom("a")}, make_atom("b"));

        assert(equal_forms(l_lhs, l_rhs));

        PL_discard_foreign_frame(l_frame);
    };

    // neq in structure
    {
        fid_t l_frame = PL_open_foreign_frame();

        std::map<std::string, term_t> l_var_alist;

        term_t l_lhs = make_list({make_atom("a")}, make_atom("b"));
        term_t l_rhs = make_list({make_atom("a"), make_atom("b")});

        assert(!equal_forms(l_lhs, l_rhs));

        PL_discard_foreign_frame(l_frame);
    };

    // neq in structure
    {
        fid_t l_frame = PL_open_foreign_frame();

        std::map<std::string, term_t> l_var_alist;

        term_t l_lhs = make_list({make_atom("a")}, make_atom("b"));
        term_t l_rhs = make_list({make_atom("a")});

        assert(!equal_forms(l_lhs, l_rhs));

        PL_discard_foreign_frame(l_frame);
    };

    // neq in structure
    {
        fid_t l_frame = PL_open_foreign_frame();

        std::map<std::string, term_t> l_var_alist;

        term_t l_lhs = make_nil();
        term_t l_rhs = make_list({make_atom("a")});

        assert(!equal_forms(l_lhs, l_rhs));

        PL_discard_foreign_frame(l_frame);
    };

    // two-element list eq
    {
        fid_t l_frame = PL_open_foreign_frame();

        std::map<std::string, term_t> l_var_alist;

        term_t l_lhs = make_list({make_atom("a"), make_atom("b")});
        term_t l_rhs = make_list({make_atom("a"), make_atom("b")});

        assert(equal_forms(l_lhs, l_rhs));

        PL_discard_foreign_frame(l_frame);
    };

    // two-element list neq
    {
        fid_t l_frame = PL_open_foreign_frame();

        std::map<std::string, term_t> l_var_alist;

        term_t l_lhs = make_list({make_atom("a"), make_atom("b")});
        term_t l_rhs = make_list({make_atom("a"), make_atom("c")});

        assert(!equal_forms(l_lhs, l_rhs));

        PL_discard_foreign_frame(l_frame);
    };

    // neq structures
    {
        fid_t l_frame = PL_open_foreign_frame();

        std::map<std::string, term_t> l_var_alist;

        term_t l_lhs = make_list({make_atom("a")});
        term_t l_rhs = make_var("X", l_var_alist);

        assert(!equal_forms(l_lhs, l_rhs));
        assert(PL_is_variable(l_rhs)); // rhs remains var

        PL_discard_foreign_frame(l_frame);
    };

    // eq forms, nested lists
    {
        fid_t l_frame = PL_open_foreign_frame();

        std::map<std::string, term_t> l_var_alist;

        term_t l_lhs = make_list({
                                     make_list({
                                                   make_atom("a"),
                                                   make_atom("b"),
                                               },
                                               make_atom("c")),
                                     make_atom("d"),
                                 },
                                 make_atom("e"));
        term_t l_rhs = make_list({
                                     make_list({
                                                   make_atom("a"),
                                                   make_atom("b"),
                                               },
                                               make_atom("c")),
                                     make_atom("d"),
                                 },
                                 make_atom("e"));

        assert(equal_forms(l_lhs, l_rhs));

        PL_discard_foreign_frame(l_frame);
    };

    // neq forms, nested lists
    {
        fid_t l_frame = PL_open_foreign_frame();

        std::map<std::string, term_t> l_var_alist;

        term_t l_lhs = make_list({
                                     make_list({
                                                   make_atom("a"),
                                                   make_atom("b1"),
                                               },
                                               make_atom("c")),
                                     make_atom("d"),
                                 },
                                 make_atom("e"));
        term_t l_rhs = make_list({
                                     make_list({
                                                   make_atom("a"),
                                                   make_atom("b"),
                                               },
                                               make_atom("c")),
                                     make_atom("d"),
                                 },
                                 make_atom("e"));

        assert(!equal_forms(l_lhs, l_rhs));

        PL_discard_foreign_frame(l_frame);
    };

    // neq forms, nested lists
    {
        fid_t l_frame = PL_open_foreign_frame();

        std::map<std::string, term_t> l_var_alist;

        term_t l_lhs = make_list({
                                     make_list({
                                                   make_atom("a"),
                                                   make_atom("b"),
                                               },
                                               make_var("X", l_var_alist)),
                                     make_atom("d"),
                                 },
                                 make_atom("e"));
        term_t l_rhs = make_list({
                                     make_list({
                                                   make_atom("a"),
                                                   make_atom("b"),
                                               },
                                               make_atom("c")),
                                     make_atom("d"),
                                 },
                                 make_atom("e"));

        assert(!equal_forms(l_lhs, l_rhs));

        PL_discard_foreign_frame(l_frame);
    };

    // eq forms, nested lists
    {
        fid_t l_frame = PL_open_foreign_frame();

        std::map<std::string, term_t> l_var_alist;

        term_t l_lhs = make_list({
                                     make_list({
                                                   make_atom("a"),
                                                   make_atom("b"),
                                               },
                                               make_var("X", l_var_alist)),
                                     make_atom("d"),
                                 },
                                 make_atom("e"));
        term_t l_rhs = make_list({
                                     make_list({
                                                   make_atom("a"),
                                                   make_atom("b"),
                                               },
                                               make_var("Y", l_var_alist)),
                                     make_atom("d"),
                                 },
                                 make_atom("e"));

        assert(equal_forms(l_lhs, l_rhs));

        PL_discard_foreign_frame(l_frame);
    };

    // neq forms, nested lists
    {
        fid_t l_frame = PL_open_foreign_frame();

        std::map<std::string, term_t> l_var_alist;

        term_t l_lhs = make_list({
                                     make_list({
                                                   make_atom("a"),
                                                   make_var("Z", l_var_alist),
                                               },
                                               make_var("X", l_var_alist)),
                                     make_atom("d"),
                                 },
                                 make_atom("e"));
        term_t l_rhs = make_list({
                                     make_list({
                                                   make_atom("a"),
                                                   make_atom("b"),
                                               },
                                               make_var("Y", l_var_alist)),
                                     make_atom("d"),
                                 },
                                 make_atom("e"));

        assert(!equal_forms(l_lhs, l_rhs));

        PL_discard_foreign_frame(l_frame);
    };

    // eq forms, lists of vars
    {
        fid_t l_frame = PL_open_foreign_frame();

        std::map<std::string, term_t> l_var_alist;

        term_t l_lhs = make_list({make_var("X", l_var_alist), make_var("Y", l_var_alist)});
        term_t l_rhs = make_list({make_var("Z", l_var_alist), make_var("W", l_var_alist)});

        assert(equal_forms(l_lhs, l_rhs));

        PL_discard_foreign_frame(l_frame);
    };

    // neq forms, lists of vars
    {
        fid_t l_frame = PL_open_foreign_frame();

        std::map<std::string, term_t> l_var_alist;

        term_t l_lhs = make_list({make_var("X", l_var_alist), make_var("X", l_var_alist)});
        term_t l_rhs = make_list({make_var("Z", l_var_alist), make_var("W", l_var_alist)});

        assert(!equal_forms(l_lhs, l_rhs));

        PL_discard_foreign_frame(l_frame);
    };

    // eq forms, lists of vars
    {
        fid_t l_frame = PL_open_foreign_frame();

        std::map<std::string, term_t> l_var_alist;

        term_t l_lhs = make_list({make_var("X", l_var_alist), make_var("X", l_var_alist)});
        term_t l_rhs = make_list({make_var("Z", l_var_alist), make_var("Z", l_var_alist)});

        assert(equal_forms(l_lhs, l_rhs));

        PL_discard_foreign_frame(l_frame);
    };

    // neq forms, lists of vars
    {
        fid_t l_frame = PL_open_foreign_frame();

        std::map<std::string, term_t> l_var_alist;

        term_t l_lhs = make_list({make_var("X", l_var_alist), make_var("X", l_var_alist), make_atom("a")});
        term_t l_rhs = make_list({make_var("Z", l_var_alist), make_var("Z", l_var_alist)});

        assert(!equal_forms(l_lhs, l_rhs));

        PL_discard_foreign_frame(l_frame);
    };

    // eq forms, lists of vars
    {
        fid_t l_frame = PL_open_foreign_frame();

        std::map<std::string, term_t> l_var_alist;

        term_t l_lhs = make_list({make_var("X", l_var_alist), make_var("X", l_var_alist), make_atom("a")});
        term_t l_rhs = make_list({make_var("Z", l_var_alist), make_var("Z", l_var_alist), make_atom("a")});

        assert(equal_forms(l_lhs, l_rhs));

        PL_discard_foreign_frame(l_frame);
    };

    // eq forms, lists of vars
    {
        fid_t l_frame = PL_open_foreign_frame();

        std::map<std::string, term_t> l_var_alist;

        term_t l_lhs = make_list({make_var("X", l_var_alist), make_var("X", l_var_alist), make_atom("a"), make_var("X", l_var_alist)});
        term_t l_rhs = make_list({make_var("Z", l_var_alist), make_var("Z", l_var_alist), make_atom("a"), make_var("Z", l_var_alist)});

        assert(equal_forms(l_lhs, l_rhs));

        PL_discard_foreign_frame(l_frame);
    };

    // neq forms, lists of vars
    {
        fid_t l_frame = PL_open_foreign_frame();

        std::map<std::string, term_t> l_var_alist;

        term_t l_lhs = make_list({make_var("X", l_var_alist), make_var("X", l_var_alist), make_atom("a"), make_var("X", l_var_alist)});
        term_t l_rhs = make_list({make_var("Z", l_var_alist), make_var("Z", l_var_alist), make_atom("a"), make_var("Y", l_var_alist)});

        assert(!equal_forms(l_lhs, l_rhs));

        PL_discard_foreign_frame(l_frame);
    };

    // eq forms, lists of vars
    {
        fid_t l_frame = PL_open_foreign_frame();

        std::map<std::string, term_t> l_var_alist;

        term_t l_lhs = make_list({
            make_atom("abc"),
            make_list({
                          make_var("X", l_var_alist),
                      },
                      make_var("T", l_var_alist)),
            make_var("Y", l_var_alist),
            make_var("Y", l_var_alist),
            make_atom("def"),
        });
        term_t l_rhs = make_list({
            make_atom("abc"),
            make_list({
                          make_var("A", l_var_alist),
                      },
                      make_var("B", l_var_alist)),
            make_var("C", l_var_alist),
            make_var("C", l_var_alist),
            make_atom("def"),
        });

        assert(equal_forms(l_lhs, l_rhs));

        PL_discard_foreign_frame(l_frame);
    };

    // neq forms, lists of vars
    {
        fid_t l_frame = PL_open_foreign_frame();

        std::map<std::string, term_t> l_var_alist;

        term_t l_lhs = make_list({
            make_atom("abc"),
            make_list({
                          make_var("X", l_var_alist),
                      },
                      make_var("T", l_var_alist)),
            make_var("Y", l_var_alist),
            make_var("Y", l_var_alist),
            make_atom("def"),
        });
        term_t l_rhs = make_list({
            make_atom("abc"),
            make_list({
                          make_var("A", l_var_alist),
                      },
                      make_var("B", l_var_alist)),
            make_var("C", l_var_alist),
            make_var("C", l_var_alist),
            make_var("D", l_var_alist),
        });

        assert(!equal_forms(l_lhs, l_rhs));

        PL_discard_foreign_frame(l_frame);
    };

    // eq forms, lists of vars
    {
        fid_t l_frame = PL_open_foreign_frame();

        std::map<std::string, term_t> l_var_alist;

        term_t l_lhs = make_list({
            make_atom("abc"),
            make_list({
                          make_var("X", l_var_alist),
                          make_var("T", l_var_alist),
                      },
                      make_var("T", l_var_alist)),
            make_var("Y", l_var_alist),
            make_var("Y", l_var_alist),
            make_atom("def"),
        });
        term_t l_rhs = make_list({
            make_atom("abc"),
            make_list({
                          make_var("A", l_var_alist),
                          make_var("B", l_var_alist),
                      },
                      make_var("B", l_var_alist)),
            make_var("C", l_var_alist),
            make_var("C", l_var_alist),
            make_atom("def"),
        });

        assert(equal_forms(l_lhs, l_rhs));

        PL_discard_foreign_frame(l_frame);
    };

    // neq forms, lists of vars
    {
        fid_t l_frame = PL_open_foreign_frame();

        std::map<std::string, term_t> l_var_alist;

        term_t l_lhs = make_list({
            make_atom("abc"),
            make_list({
                          make_var("X", l_var_alist),
                          make_var("Y", l_var_alist),
                      },
                      make_var("T", l_var_alist)),
            make_var("Y", l_var_alist),
            make_var("Y", l_var_alist),
            make_atom("def"),
        });
        term_t l_rhs = make_list({
            make_atom("abc"),
            make_list({
                          make_var("A", l_var_alist),
                          make_var("B", l_var_alist),
                      },
                      make_var("B", l_var_alist)),
            make_var("C", l_var_alist),
            make_var("C", l_var_alist),
            make_atom("def"),
        });

        assert(!equal_forms(l_lhs, l_rhs));
        assert(!PL_is_variable(make_var("X", l_var_alist)));
        assert(!PL_is_variable(make_var("Y", l_var_alist)));
        assert(!PL_is_variable(make_var("A", l_var_alist)));
        assert(!PL_is_variable(make_var("B", l_var_alist)));
        assert(PL_is_variable(make_var("T", l_var_alist))); // T should STILL be var since counterpart was atom and form eq fails.
        assert(PL_is_variable(make_var("C", l_var_alist))); // form eq does not make it to checking C thus it remains var.

        PL_discard_foreign_frame(l_frame);
    };
}

static void test_axiom_statement_equal()
{
    constexpr bool ENABLE_DEBUG_LOGS = true;

    fid_t l_frame = PL_open_foreign_frame();

    using unilog::axiom_statement;
    using unilog::statement;

    std::map<std::string, term_t> l_var_alist;

    data_points<std::pair<axiom_statement, axiom_statement>, bool> l_data_points =
        {
            {
                {
                    axiom_statement{
                        .m_tag = make_atom("a0"),
                        .m_theorem = make_list({
                            make_atom("claims"),
                            make_atom("leon"),
                            make_atom("x"),
                        }),
                    },
                    axiom_statement{
                        .m_tag = make_atom("a1"),
                        .m_theorem = make_list({
                            make_atom("claims"),
                            make_atom("leon"),
                            make_atom("x"),
                        }),
                    },
                },
                false,
            },
            {
                {
                    axiom_statement{
                        .m_tag = make_atom("a0"),
                        .m_theorem = make_list({
                            make_atom("claims"),
                            make_atom("leon"),
                            make_atom("x"),
                        }),
                    },
                    axiom_statement{
                        .m_tag = make_atom("a0"),
                        .m_theorem = make_list({
                            make_atom("claims"),
                            make_atom("leon1"),
                            make_atom("x"),
                        }),
                    },
                },
                false,
            },
            {
                {
                    axiom_statement{
                        .m_tag = make_atom("a0"),
                        .m_theorem = make_list({
                            make_atom("claims"),
                            make_atom("leon"),
                            make_atom("x"),
                        }),
                    },
                    axiom_statement{
                        .m_tag = make_atom("a0"),
                        .m_theorem = make_list({
                            make_atom("claims"),
                            make_atom("leon"),
                            make_atom("x"),
                        }),
                    },
                },
                true,
            },
            {
                {
                    axiom_statement{
                        .m_tag = make_atom("a0"),
                        .m_theorem = make_list({
                            make_var("A", l_var_alist),
                            make_atom("leon"),
                            make_atom("x"),
                        }),
                    },
                    axiom_statement{
                        .m_tag = make_atom("a0"),
                        .m_theorem = make_list({
                            make_var("B", l_var_alist),
                            make_atom("leon"),
                            make_atom("x"),
                        }),
                    },
                },
                true,
            },
            {
                {
                    axiom_statement{
                        .m_tag = make_var("A", l_var_alist),
                        .m_theorem = make_list({
                            make_var("A", l_var_alist),
                            make_atom("leon"),
                            make_atom("x"),
                        }),
                    },
                    axiom_statement{
                        .m_tag = make_var("B", l_var_alist),
                        .m_theorem = make_list({
                            make_var("B", l_var_alist),
                            make_atom("leon"),
                            make_atom("x"),
                        }),
                    },
                },
                true,
            },
            {
                {
                    axiom_statement{
                        .m_tag = make_var("A", l_var_alist),
                        .m_theorem = make_list({
                            make_var("A", l_var_alist),
                            make_atom("leon"),
                            make_atom("x"),
                        }),
                    },
                    axiom_statement{
                        .m_tag = make_var("B", l_var_alist),
                        .m_theorem = make_list({
                            make_var("C", l_var_alist),
                            make_atom("leon"),
                            make_atom("x"),
                        }),
                    },
                },
                false,
            },
            {
                {
                    axiom_statement{
                        .m_tag = make_atom("a"),
                        .m_theorem = make_list({
                            make_var("A", l_var_alist),
                            make_atom("leon"),
                            make_atom("x"),
                        }),
                    },
                    axiom_statement{
                        .m_tag = make_atom("a"),
                        .m_theorem = make_list({
                            make_var("C", l_var_alist),
                            make_atom("leon"),
                            make_atom("x"),
                        }),
                    },
                },
                true,
            },
            {
                {
                    axiom_statement{
                        .m_tag = make_list({make_var("A", l_var_alist)}),
                        .m_theorem = make_list({
                            make_var("A", l_var_alist),
                            make_atom("leon"),
                            make_atom("x"),
                        }),
                    },
                    axiom_statement{
                        .m_tag = make_list({make_var("B", l_var_alist)}),
                        .m_theorem = make_list({
                            make_var("B", l_var_alist),
                            make_atom("leon"),
                            make_atom("x"),
                        }),
                    },
                },
                true,
            },
            {
                {
                    axiom_statement{
                        .m_tag = make_list({make_var("A", l_var_alist)}),
                        .m_theorem = make_list({
                            make_var("A", l_var_alist),
                            make_var("A", l_var_alist),
                            make_atom("x"),
                        }),
                    },
                    axiom_statement{
                        .m_tag = make_list({make_var("B", l_var_alist)}),
                        .m_theorem = make_list({
                            make_var("B", l_var_alist),
                            make_var("C", l_var_alist),
                            make_atom("x"),
                        }),
                    },
                },
                false,
            },
        };

    for (const auto &[l_pair, l_value] : l_data_points)
    {
        // do the equality comparison
        assert((l_pair.first == l_pair.second) == l_value);

        LOG("success, axiom_statement op== test" << std::endl);
    }

    PL_discard_foreign_frame(l_frame);
}

static void test_redir_statement_equal()
{
    constexpr bool ENABLE_DEBUG_LOGS = true;

    fid_t l_frame = PL_open_foreign_frame();

    using unilog::redir_statement;
    using unilog::statement;

    std::map<std::string, term_t> l_var_alist;

    data_points<std::pair<redir_statement, redir_statement>, bool> l_data_points =
        {
            {
                {
                    redir_statement{
                        .m_tag = make_list({
                            make_atom("a0"),
                            make_var("A", l_var_alist),
                            make_var("B", l_var_alist),
                            make_var("C", l_var_alist),
                        }),
                        .m_guide = make_list({
                            make_atom("sub"),
                            make_var("A", l_var_alist),
                            make_var("B", l_var_alist),
                            make_var("C", l_var_alist),
                        }),
                    },
                    redir_statement{
                        .m_tag = make_list({
                            make_atom("a0"),
                            make_var("D", l_var_alist),
                            make_var("E", l_var_alist),
                            make_var("F", l_var_alist),
                        }),
                        .m_guide = make_list({
                            make_atom("sub"),
                            make_var("D", l_var_alist),
                            make_var("E", l_var_alist),
                            make_var("F", l_var_alist),
                        }),
                    },
                },
                true,
            },
            {
                {
                    redir_statement{
                        .m_tag = make_list({
                            make_atom("a0"),
                            make_var("B", l_var_alist),
                            make_var("B", l_var_alist),
                            make_var("C", l_var_alist),
                        }),
                        .m_guide = make_list({
                            make_atom("sub"),
                            make_var("A", l_var_alist),
                            make_var("B", l_var_alist),
                            make_var("C", l_var_alist),
                        }),
                    },
                    redir_statement{
                        .m_tag = make_list({
                            make_atom("a0"),
                            make_var("D", l_var_alist),
                            make_var("E", l_var_alist),
                            make_var("F", l_var_alist),
                        }),
                        .m_guide = make_list({
                            make_atom("sub"),
                            make_var("D", l_var_alist),
                            make_var("E", l_var_alist),
                            make_var("F", l_var_alist),
                        }),
                    },
                },
                false,
            },
            {
                {
                    redir_statement{
                        .m_tag = make_list({
                            make_atom("a0"),
                            make_var("A", l_var_alist),
                            make_var("B", l_var_alist),
                            make_var("C", l_var_alist),
                        }),
                        .m_guide = make_list({
                            make_atom("sub"),
                            make_var("D", l_var_alist),
                            make_var("B", l_var_alist),
                            make_var("C", l_var_alist),
                        }),
                    },
                    redir_statement{
                        .m_tag = make_list({
                            make_atom("a0"),
                            make_var("E", l_var_alist),
                            make_var("F", l_var_alist),
                            make_var("G", l_var_alist),
                        }),
                        .m_guide = make_list({
                            make_atom("sub"),
                            make_var("E", l_var_alist),
                            make_var("F", l_var_alist),
                            make_var("G", l_var_alist),
                        }),
                    },
                },
                false,
            },
            {
                {
                    redir_statement{
                        .m_tag = make_list({
                            make_var("X", l_var_alist),
                            make_var("A", l_var_alist),
                            make_var("B", l_var_alist),
                            make_var("C", l_var_alist),
                        }),
                        .m_guide = make_list({
                            make_atom("sub"),
                            make_var("X", l_var_alist),
                            make_var("B", l_var_alist),
                            make_var("C", l_var_alist),
                        }),
                    },
                    redir_statement{
                        .m_tag = make_list({
                            make_var("Y", l_var_alist),
                            make_var("E", l_var_alist),
                            make_var("F", l_var_alist),
                            make_var("G", l_var_alist),
                        }),
                        .m_guide = make_list({
                            make_atom("sub"),
                            make_var("E", l_var_alist),
                            make_var("F", l_var_alist),
                            make_var("G", l_var_alist),
                        }),
                    },
                },
                false,
            },
            {
                {
                    redir_statement{
                        .m_tag = make_list({
                            make_var("X", l_var_alist),
                            make_var("A", l_var_alist),
                            make_var("B", l_var_alist),
                            make_var("C", l_var_alist),
                        }),
                        .m_guide = make_list({
                            make_atom("sub"),
                            make_var("X", l_var_alist),
                            make_var("B", l_var_alist),
                            make_var("C", l_var_alist),
                        }),
                    },
                    redir_statement{
                        .m_tag = make_list({
                            make_var("Y", l_var_alist),
                            make_var("E", l_var_alist),
                            make_var("F", l_var_alist),
                            make_var("G", l_var_alist),
                        }),
                        .m_guide = make_list({
                            make_atom("sub"),
                            make_var("Y", l_var_alist),
                            make_var("F", l_var_alist),
                            make_var("G", l_var_alist),
                        }),
                    },
                },
                true,
            },
            {
                {
                    redir_statement{
                        .m_tag = make_list({
                                               make_var("X", l_var_alist),
                                               make_var("A", l_var_alist),
                                           },
                                           make_var("B", l_var_alist)),
                        .m_guide = make_list({
                                                 make_atom("sub"),
                                                 make_var("A", l_var_alist),
                                             },
                                             make_var("B", l_var_alist)),
                    },
                    redir_statement{
                        .m_tag = make_list({
                                               make_var("Y", l_var_alist),
                                               make_var("C", l_var_alist),
                                           },
                                           make_var("D", l_var_alist)),
                        .m_guide = make_list({
                                                 make_atom("sub"),
                                                 make_var("C", l_var_alist),
                                             },
                                             make_var("D", l_var_alist)),
                    },
                },
                true,
            },
        };

    for (const auto &[l_pair, l_value] : l_data_points)
    {
        // do the equality comparison
        assert((l_pair.first == l_pair.second) == l_value);

        LOG("success, redir_statement op== test" << std::endl);
    }

    PL_discard_foreign_frame(l_frame);
}

static void test_infer_statement_equal()
{
    constexpr bool ENABLE_DEBUG_LOGS = true;

    fid_t l_frame = PL_open_foreign_frame();

    using unilog::infer_statement;
    using unilog::statement;

    std::map<std::string, term_t> l_var_alist;

    data_points<std::pair<infer_statement, infer_statement>, bool> l_data_points =
        {
            {
                {
                    infer_statement{
                        .m_tag = make_list({
                            make_atom("a0"),
                            make_var("A", l_var_alist),
                            make_var("B", l_var_alist),
                            make_var("C", l_var_alist),
                        }),
                        .m_guide = make_list({
                            make_atom("sub"),
                            make_var("A", l_var_alist),
                            make_var("B", l_var_alist),
                            make_var("C", l_var_alist),
                        }),
                    },
                    infer_statement{
                        .m_tag = make_list({
                            make_atom("a0"),
                            make_var("D", l_var_alist),
                            make_var("E", l_var_alist),
                            make_var("F", l_var_alist),
                        }),
                        .m_guide = make_list({
                            make_atom("sub"),
                            make_var("D", l_var_alist),
                            make_var("E", l_var_alist),
                            make_var("F", l_var_alist),
                        }),
                    },
                },
                true,
            },
            {
                {
                    infer_statement{
                        .m_tag = make_list({
                            make_atom("a0"),
                            make_var("B", l_var_alist),
                            make_var("B", l_var_alist),
                            make_var("C", l_var_alist),
                        }),
                        .m_guide = make_list({
                            make_atom("sub"),
                            make_var("A", l_var_alist),
                            make_var("B", l_var_alist),
                            make_var("C", l_var_alist),
                        }),
                    },
                    infer_statement{
                        .m_tag = make_list({
                            make_atom("a0"),
                            make_var("D", l_var_alist),
                            make_var("E", l_var_alist),
                            make_var("F", l_var_alist),
                        }),
                        .m_guide = make_list({
                            make_atom("sub"),
                            make_var("D", l_var_alist),
                            make_var("E", l_var_alist),
                            make_var("F", l_var_alist),
                        }),
                    },
                },
                false,
            },
            {
                {
                    infer_statement{
                        .m_tag = make_list({
                            make_atom("a0"),
                            make_var("A", l_var_alist),
                            make_var("B", l_var_alist),
                            make_var("C", l_var_alist),
                        }),
                        .m_guide = make_list({
                            make_atom("sub"),
                            make_var("D", l_var_alist),
                            make_var("B", l_var_alist),
                            make_var("C", l_var_alist),
                        }),
                    },
                    infer_statement{
                        .m_tag = make_list({
                            make_atom("a0"),
                            make_var("E", l_var_alist),
                            make_var("F", l_var_alist),
                            make_var("G", l_var_alist),
                        }),
                        .m_guide = make_list({
                            make_atom("sub"),
                            make_var("E", l_var_alist),
                            make_var("F", l_var_alist),
                            make_var("G", l_var_alist),
                        }),
                    },
                },
                false,
            },
            {
                {
                    infer_statement{
                        .m_tag = make_list({
                            make_var("X", l_var_alist),
                            make_var("A", l_var_alist),
                            make_var("B", l_var_alist),
                            make_var("C", l_var_alist),
                        }),
                        .m_guide = make_list({
                            make_atom("sub"),
                            make_var("X", l_var_alist),
                            make_var("B", l_var_alist),
                            make_var("C", l_var_alist),
                        }),
                    },
                    infer_statement{
                        .m_tag = make_list({
                            make_var("Y", l_var_alist),
                            make_var("E", l_var_alist),
                            make_var("F", l_var_alist),
                            make_var("G", l_var_alist),
                        }),
                        .m_guide = make_list({
                            make_atom("sub"),
                            make_var("E", l_var_alist),
                            make_var("F", l_var_alist),
                            make_var("G", l_var_alist),
                        }),
                    },
                },
                false,
            },
            {
                {
                    infer_statement{
                        .m_tag = make_list({
                            make_var("X", l_var_alist),
                            make_var("A", l_var_alist),
                            make_var("B", l_var_alist),
                            make_var("C", l_var_alist),
                        }),
                        .m_guide = make_list({
                            make_atom("sub"),
                            make_var("X", l_var_alist),
                            make_var("B", l_var_alist),
                            make_var("C", l_var_alist),
                        }),
                    },
                    infer_statement{
                        .m_tag = make_list({
                            make_var("Y", l_var_alist),
                            make_var("E", l_var_alist),
                            make_var("F", l_var_alist),
                            make_var("G", l_var_alist),
                        }),
                        .m_guide = make_list({
                            make_atom("sub"),
                            make_var("Y", l_var_alist),
                            make_var("F", l_var_alist),
                            make_var("G", l_var_alist),
                        }),
                    },
                },
                true,
            },
            {
                {
                    infer_statement{
                        .m_tag = make_list({
                                               make_var("X", l_var_alist),
                                               make_var("A", l_var_alist),
                                           },
                                           make_var("B", l_var_alist)),
                        .m_guide = make_list({
                                                 make_atom("sub"),
                                                 make_var("A", l_var_alist),
                                             },
                                             make_var("B", l_var_alist)),
                    },
                    infer_statement{
                        .m_tag = make_list({
                                               make_var("Y", l_var_alist),
                                               make_var("C", l_var_alist),
                                           },
                                           make_var("D", l_var_alist)),
                        .m_guide = make_list({
                                                 make_atom("sub"),
                                                 make_var("C", l_var_alist),
                                             },
                                             make_var("D", l_var_alist)),
                    },
                },
                true,
            },
            {
                {
                    infer_statement{
                        .m_tag = make_list({
                                               make_var("X", l_var_alist),
                                               make_atom("abc"),
                                           },
                                           make_var("B", l_var_alist)),
                        .m_guide = make_list({
                                                 make_atom("sub"),
                                                 make_var("A", l_var_alist),
                                             },
                                             make_var("B", l_var_alist)),
                    },
                    infer_statement{
                        .m_tag = make_list({
                                               make_var("Y", l_var_alist),
                                               make_atom("abc"),
                                           },
                                           make_var("D", l_var_alist)),
                        .m_guide = make_list({
                                                 make_atom("sub"),
                                                 make_var("C", l_var_alist),
                                             },
                                             make_var("D", l_var_alist)),
                    },
                },
                true,
            },
            {
                {
                    infer_statement{
                        .m_tag = make_list({
                                               make_var("X", l_var_alist),
                                               make_atom("abc"),
                                           },
                                           make_var("B", l_var_alist)),
                        .m_guide = make_list({
                                                 make_atom("sub"),
                                                 make_var("A", l_var_alist),
                                             },
                                             make_var("B", l_var_alist)),
                    },
                    infer_statement{
                        .m_tag = make_list({
                                               make_var("Y", l_var_alist),
                                               make_atom("abc1"),
                                           },
                                           make_var("D", l_var_alist)),
                        .m_guide = make_list({
                                                 make_atom("sub"),
                                                 make_var("C", l_var_alist),
                                             },
                                             make_var("D", l_var_alist)),
                    },
                },
                false,
            },
        };

    for (const auto &[l_pair, l_value] : l_data_points)
    {
        // do the equality comparison
        assert((l_pair.first == l_pair.second) == l_value);

        LOG("success, redir_statement op== test" << std::endl);
    }

    PL_discard_foreign_frame(l_frame);
}

static void test_refer_statement_equal()
{
    constexpr bool ENABLE_DEBUG_LOGS = true;

    fid_t l_frame = PL_open_foreign_frame();

    using unilog::refer_statement;
    using unilog::statement;

    std::map<std::string, term_t> l_var_alist;

    data_points<std::pair<refer_statement, refer_statement>, bool> l_data_points =
        {
            {
                {
                    refer_statement{
                        .m_tag = make_atom("leon"),
                        .m_file_path = make_atom("./inlcude/leon.u"),
                    },
                    refer_statement{
                        .m_tag = make_atom("leon"),
                        .m_file_path = make_atom("./inlcude/leon.u"),
                    },
                },
                true,
            },
            {
                {
                    refer_statement{
                        .m_tag = make_atom("leon"),
                        .m_file_path = make_var("X", l_var_alist),
                    },
                    refer_statement{
                        .m_tag = make_atom("leon"),
                        .m_file_path = make_var("Y", l_var_alist),
                    },
                },
                true,
            },
            {
                {
                    refer_statement{
                        .m_tag = make_list({make_atom("abc"), make_var("X", l_var_alist)}),
                        .m_file_path = make_var("X", l_var_alist),
                    },
                    refer_statement{
                        .m_tag = make_list({make_atom("abc"), make_var("Y", l_var_alist)}),
                        .m_file_path = make_var("Y", l_var_alist),
                    },
                },
                true,
            },
            {
                {
                    refer_statement{
                        .m_tag = make_list({make_atom("abc"), make_var("X", l_var_alist)}),
                        .m_file_path = make_var("X", l_var_alist),
                    },
                    refer_statement{
                        .m_tag = make_list({make_atom("abc"), make_var("Y", l_var_alist)}),
                        .m_file_path = make_var("Z", l_var_alist),
                    },
                },
                false,
            },
        };

    for (const auto &[l_pair, l_value] : l_data_points)
    {
        // do the equality comparison
        assert((l_pair.first == l_pair.second) == l_value);

        LOG("success, axiom_statement op== test" << std::endl);
    }

    PL_discard_foreign_frame(l_frame);
}

static void test_parser_extract_prolog_expression()
{
    fid_t l_function_frame = PL_open_foreign_frame();

    constexpr bool ENABLE_DEBUG_LOGS = true;

    std::map<std::string, term_t> l_data_points_var_alist;

    data_points<std::string, term_t>
        l_test_cases =
            {
                {
                    "if",
                    make_atom("if"),
                },
                {
                    "add",
                    make_atom("add"),
                },
                {
                    "\'\'",
                    make_atom(""),
                },
                {
                    "\'abc123\'",
                    make_atom("abc123"),
                },
                {
                    "abc123\n",
                    make_atom("abc123"),
                },
                {
                    "abc123/\n",
                    make_atom("abc123"),
                },
                {
                    "\'abc123/\\n\'",
                    make_atom("abc123/\n"),
                },
                {
                    "Var",
                    make_var("Var", l_data_points_var_alist),
                },
                {
                    "_",
                    make_var("_", l_data_points_var_alist),
                },
                {
                    "Var abc", // second term does not get extracted
                    make_var("Var", l_data_points_var_alist),
                },
                {
                    "[]",
                    make_nil(),
                },
                {
                    "[[]]",
                    make_list({
                        make_nil(),
                    }),
                },
                {
                    "[abc]",
                    make_list({
                        make_atom("abc"),
                    }),
                },
                {
                    "[abc abd]",
                    make_list({
                        make_atom("abc"),
                        make_atom("abd"),
                    }),
                },
                {
                    "[abc abc2]",
                    make_list({
                        make_atom("abc"),
                        make_atom("abc2"),
                    }),
                },
                {
                    "[abc \'1010\']",
                    make_list({
                        make_atom("abc"),
                        make_atom("1010"),
                    }),
                },
                {
                    "[X]",
                    make_list({
                        make_var("X", l_data_points_var_alist),
                    }),
                },
                {
                    "[X abc]",
                    make_list({
                        make_var("X", l_data_points_var_alist),
                        make_atom("abc"),
                    }),
                },
                {
                    "[a b c]",
                    make_list({
                        make_atom("a"),
                        make_atom("b"),
                        make_atom("c"),
                    }),
                },
                {
                    "[a X c]",
                    make_list({
                        make_atom("a"),
                        make_var("X", l_data_points_var_alist),
                        make_atom("c"),
                    }),
                },
                {
                    "[a X X]",
                    make_list({
                        make_atom("a"),
                        make_var("X", l_data_points_var_alist),
                        make_var("X", l_data_points_var_alist),
                    }),
                },
                {
                    "[a 'X' X]",
                    make_list({
                        make_atom("a"),
                        make_atom("X"),
                        make_var("X", l_data_points_var_alist),
                    }),
                },
                {
                    "[[a]]",
                    make_list({
                        make_list({
                            make_atom("a"),
                        }),
                    }),
                },
                {
                    "[[a b]]",
                    make_list({
                        make_list({
                            make_atom("a"),
                            make_atom("b"),
                        }),
                    }),
                },
                {
                    "[[a b] c]",
                    make_list({
                        make_list({
                            make_atom("a"),
                            make_atom("b"),
                        }),
                        make_atom("c"),
                    }),
                },
                {
                    "[[a X] X]",
                    make_list({
                        make_list({
                            make_atom("a"),
                            make_var("X", l_data_points_var_alist),
                        }),
                        make_var("X", l_data_points_var_alist),
                    }),
                },
                {
                    "[|a]",
                    make_atom("a"),
                },
                {
                    "[a|b]",
                    make_list({
                                  make_atom("a"),
                              },
                              make_atom("b")),
                },
                {
                    "[a|Y]",
                    make_list({
                                  make_atom("a"),
                              },
                              make_var("Y", l_data_points_var_alist)),
                },
                {
                    "[a|[]]",
                    make_list({
                        make_atom("a"),
                    }),
                },
                {
                    "[a|[b|c]]",
                    make_list({
                                  make_atom("a"),
                                  make_atom("b"),
                              },
                              make_atom("c")),
                },
                {
                    "[a|[b|X]]",
                    make_list({
                                  make_atom("a"),
                                  make_atom("b"),
                              },
                              make_var("X", l_data_points_var_alist)),
                },
                {
                    "['1'|[b|X]]",
                    make_list({
                                  make_atom("1"),
                                  make_atom("b"),
                              },
                              make_var("X", l_data_points_var_alist)),
                },
                {
                    "[a\nb]",
                    make_list({
                        make_atom("a"),
                        make_atom("b"),
                    }),
                },
                {
                    "[a\tb]",
                    make_list({
                        make_atom("a"),
                        make_atom("b"),
                    }),
                },
                {
                    "[a\tb\n]",
                    make_list({
                        make_atom("a"),
                        make_atom("b"),
                    }),
                },
                {
                    "[a b c d e f g]",
                    make_list({
                        make_atom("a"),
                        make_atom("b"),
                        make_atom("c"),
                        make_atom("d"),
                        make_atom("e"),
                        make_atom("f"),
                        make_atom("g"),
                    }),
                },
                {
                    "[a\tb\n\rc  d\t\tE\nf_g]",
                    make_list({
                        make_atom("a"),
                        make_atom("b"),
                        make_atom("c"),
                        make_atom("d"),
                        make_var("E", l_data_points_var_alist),
                        make_atom("f_g"),
                    }),
                },
                {
                    "[A|B]",
                    make_list({
                                  make_var("A", l_data_points_var_alist),
                              },
                              make_var("B", l_data_points_var_alist)),
                },
                {
                    "[X X Y|X]",
                    make_list({
                                  make_var("X", l_data_points_var_alist),
                                  make_var("X", l_data_points_var_alist),
                                  make_var("Y", l_data_points_var_alist),
                              },
                              make_var("X", l_data_points_var_alist)),
                },
                {
                    "[X a [B\t|\tX\t]\t[[[a\tb\nc] d e] f g|\r\n[[]]] B | Y]",
                    make_list({
                                  make_var("X", l_data_points_var_alist),
                                  make_atom("a"),
                                  make_list({make_var("B", l_data_points_var_alist)}, make_var("X", l_data_points_var_alist)),
                                  make_list({
                                                make_list({
                                                    make_list({
                                                        make_atom("a"),
                                                        make_atom("b"),
                                                        make_atom("c"),
                                                    }),
                                                    make_atom("d"),
                                                    make_atom("e"),
                                                }),
                                                make_atom("f"),
                                                make_atom("g"),
                                            },
                                            make_list({
                                                make_nil(),
                                            })),
                                  make_var("B", l_data_points_var_alist),
                              },
                              make_var("Y", l_data_points_var_alist)),
                },
                {
                    "[a|[b|[c|[d]]]]",
                    make_list({
                        make_atom("a"),
                        make_atom("b"),
                        make_atom("c"),
                        make_atom("d"),
                    }),
                },
                {
                    "[X X X]",
                    make_list({
                        make_var("X", l_data_points_var_alist),
                        make_var("X", l_data_points_var_alist),
                        make_var("X", l_data_points_var_alist),
                    }),
                },
                {
                    "[if\n"
                    "    [add [XH|XT] [YH|YT] [ZH|ZT]]\n"
                    "    [and\n"
                    "        [add [XH] [YH] [ZH|C]]\n"
                    "        [add XT YT T]\n"
                    "        [add T [C] ZT]\n"
                    "    ]\n"
                    "]",
                    make_list({
                        make_atom("if"),
                        make_list({
                            make_atom("add"),
                            make_list({make_var("XH", l_data_points_var_alist)}, make_var("XT", l_data_points_var_alist)),
                            make_list({make_var("YH", l_data_points_var_alist)}, make_var("YT", l_data_points_var_alist)),
                            make_list({make_var("ZH", l_data_points_var_alist)}, make_var("ZT", l_data_points_var_alist)),
                        }),
                        make_list({
                            make_atom("and"),
                            make_list({
                                make_atom("add"),
                                make_list({make_var("XH", l_data_points_var_alist)}),
                                make_list({make_var("YH", l_data_points_var_alist)}),
                                make_list({make_var("ZH", l_data_points_var_alist)}, make_var("C", l_data_points_var_alist)),
                            }),
                            make_list({
                                make_atom("add"),
                                make_var("XT", l_data_points_var_alist),
                                make_var("YT", l_data_points_var_alist),
                                make_var("T", l_data_points_var_alist),
                            }),
                            make_list({
                                make_atom("add"),
                                make_var("T", l_data_points_var_alist),
                                make_list({make_var("C", l_data_points_var_alist)}),
                                make_var("ZT", l_data_points_var_alist),
                            }),
                        }),
                    }),
                },
                {
                    "['\\n\\t']",
                    make_list({
                        make_atom("\n\t"),
                    }),
                },
                {
                    "[\"\\n\\t\"]",
                    make_list({
                        make_atom("\n\t"),
                    }),
                },
                {
                    "[\"\\n\\t\'\"]",
                    make_list({
                        make_atom("\n\t\'"),
                    }),
                },
                {
                    "[a|[b|'']]",
                    make_list({
                                  make_atom("a"),
                              },
                              make_list({
                                            make_atom("b"),
                                        },
                                        make_atom(""))),
                },
                {
                    "[a # \t\r  asdas dsad dfsdfijfjgkhjk []\n|[b|'']]",
                    make_list({
                                  make_atom("a"),
                              },
                              make_list({
                                            make_atom("b"),
                                        },
                                        make_atom(""))),
                },
            };

    for (const auto &[l_key, l_value] : l_test_cases)
    {
        std::stringstream l_ss(l_key);

        // open PL stack frame
        fid_t l_frame_id = PL_open_foreign_frame();

        term_t l_exp = PL_new_term_ref();

        std::map<std::string, term_t> l_var_alist;

        unilog::extract_term_t(l_ss, l_var_alist, l_exp);

        assert(!l_ss.fail());

        assert(equal_forms(l_exp, l_value));

        // close PL stack frame
        PL_discard_foreign_frame(l_frame_id);

        LOG("success, case: \"" << l_key << "\"" << std::endl);
    }

    std::vector<std::string> l_expect_failure_inputs =
        {
            "[abc",
            "[[abc] [123]",
            "]",
            "\'",
            "\"",
            "[a|]",
            "[a|b|c]",
            "[a|b c]",
            "[a|[b|']]",
            "[;]",
            "[|]",
            // "abc]", // this is NOT an expect failure input.
            // this is because it will only try to parse the first prolog expression before a lexeme separator char.

        };

    for (const auto &l_input : l_expect_failure_inputs)
    {
        std::stringstream l_ss(l_input);

        // open PL stack frame
        fid_t l_frame_id = PL_open_foreign_frame();

        term_t l_exp = PL_new_term_ref();

        std::map<std::string, term_t> l_var_alist;

        unilog::extract_term_t(l_ss, l_var_alist, l_exp);

        assert(l_ss.fail());

        // close PL stack frame
        PL_discard_foreign_frame(l_frame_id);

        LOG("success, expected failbit, case: " << l_input << std::endl);
    }

    PL_discard_foreign_frame(l_function_frame);
}

static void test_parser_extract_axiom_statement()
{
    fid_t l_frame_id = PL_open_foreign_frame();

    constexpr bool ENABLE_DEBUG_LOGS = true;

    using unilog::axiom_statement;
    using unilog::statement;

    std::map<std::string, term_t> l_var_alist;

    data_points<std::string, statement> l_test_cases =
        {
            {
                "axiom a0 x ;",
                axiom_statement{
                    .m_tag =
                        make_atom("a0"),
                    .m_theorem =
                        make_atom("x"),
                },
            },
            {
                "axiom add_bc_0 [add [] L L];",
                axiom_statement{
                    .m_tag =
                        make_atom("add_bc_0"),
                    .m_theorem =
                        make_list({
                            make_atom("add"),
                            make_nil(),
                            make_var("L", l_var_alist),
                            make_var("L", l_var_alist),
                        }),
                },
            },
            {
                "axiom \"@tag\" [awesome X];",
                axiom_statement{
                    .m_tag =
                        make_atom("@tag"),
                    .m_theorem =
                        make_list({
                            make_atom("awesome"),
                            make_var("X", l_var_alist),
                        }),
                },
            },
            {
                "axiom tag[theorem]\n\t;",
                axiom_statement{
                    .m_tag =
                        make_atom("tag"),
                    .m_theorem =
                        make_list({
                            make_atom("theorem"),
                        }),
                },
            },
            {
                "axiom abc '123';",
                axiom_statement{
                    .m_tag =
                        make_atom("abc"),
                    .m_theorem =
                        make_atom("123"),
                },
            },
            {
                "\'axiom\' \"123\" [[[]] a \"123\"]\t\r;",
                axiom_statement{
                    .m_tag =
                        make_atom("123"),
                    .m_theorem =
                        make_list({
                            make_list({
                                make_list({

                                }),
                            }),
                            make_atom("a"),
                            make_atom("123"),
                        }),
                },
            },
            {
                "axiom \'+/bc/0\'\n"
                "[\'+\'\n"
                "    []\n"
                "    L\n"
                "    L\n"
                "]\t;",
                axiom_statement{
                    .m_tag =
                        make_atom("+/bc/0"),
                    .m_theorem =
                        make_list({
                            make_atom("+"),
                            make_list({}),
                            make_var("L", l_var_alist),
                            make_var("L", l_var_alist),
                        }),
                },
            },
            {
                "axiom \'123\'[\'! this is a \\t quotation\']\n\n;",
                axiom_statement{
                    .m_tag =
                        make_atom("123"),
                    .m_theorem =
                        make_list({
                            make_atom("! this is a \t quotation"),
                        }),
                },
            },
            {
                "\"axiom\" \'tag\' theorem;",
                axiom_statement{
                    .m_tag =
                        make_atom("tag"),
                    .m_theorem =
                        make_atom("theorem"),
                },
            },
            {
                "\'axiom\' \"tag\" theorem\t;",
                axiom_statement{
                    .m_tag =
                        make_atom("tag"),
                    .m_theorem =
                        make_atom("theorem"),
                },
            },
        };

    for (const auto &[l_key, l_value] : l_test_cases)
    {
        fid_t l_case_frame = PL_open_foreign_frame();

        std::stringstream l_ss(l_key);

        statement l_statement;
        l_ss >> l_statement;

        // make sure the stringstream is not in failstate
        assert(!l_ss.fail());
        assert(std::holds_alternative<axiom_statement>(l_statement));
        assert(l_statement == l_value);

        LOG("success, case: \"" << l_key << "\"" << std::endl);

        PL_discard_foreign_frame(l_case_frame);
    }

    std::vector<std::string> l_fail_cases =
        {
            "",
            "abc",
            "X",
            ";",
            "[]",
            "axiom",
            "axiom a0",
            "axiom a0;",
            "axiom a0 x",
            "axiom a0 x y;",
            "\'axiom\'",
            "axiom \'a0\'",
            "axiom \'a0\';",
            "axiom \'a0\' \'x\'",
            "axiom \'a0\' \'x\' \'y\';",
        };

    for (const auto &l_input : l_fail_cases)
    {
        fid_t l_case_frame = PL_open_foreign_frame();

        std::stringstream l_ss(l_input);

        statement l_statement;
        l_ss >> l_statement;

        // ensure failure of extraction
        assert(l_ss.fail());

        LOG("success, case: expected failure extracting axiom_statement: " << l_input << std::endl);

        PL_discard_foreign_frame(l_case_frame);
    }

    PL_discard_foreign_frame(l_frame_id);
}

static void test_parser_extract_redir_statement()
{
    fid_t l_frame = PL_open_foreign_frame();

    constexpr bool ENABLE_DEBUG_LOGS = true;

    using unilog::redir_statement;
    using unilog::statement;

    std::map<std::string, term_t> l_var_alist;

    data_points<std::string, statement> l_test_cases =
        {
            {
                "redir g_add_bc\n"
                "[gor\n"
                "    add_bc_0\n"
                "    add_bc_1\n"
                "    add_bc_2\n"
                "    add_bc_3\n"
                "    add_bc_4\n"
                "    add_bc_5\n"
                "]\n;",
                redir_statement{
                    .m_tag = make_atom("g_add_bc"),
                    .m_guide = make_list({
                        make_atom("gor"),
                        make_atom("add_bc_0"),
                        make_atom("add_bc_1"),
                        make_atom("add_bc_2"),
                        make_atom("add_bc_3"),
                        make_atom("add_bc_4"),
                        make_atom("add_bc_5"),
                    }),
                },
            },
            {
                "redir g0[bind K [theorem a0]]\t;",
                redir_statement{
                    .m_tag = make_atom("g0"),
                    .m_guide = make_list({
                        make_atom("bind"),
                        make_var("K", l_var_alist),
                        make_list({
                            make_atom("theorem"),
                            make_atom("a0"),
                        }),
                    }),
                },
            },
            {
                "redir \"g\" [sub thm [theorem a0] [theorem a1]];",
                redir_statement{
                    .m_tag = make_atom("g"),
                    .m_guide = make_list({
                        make_atom("sub"),
                        make_atom("thm"),
                        make_list({
                            make_atom("theorem"),
                            make_atom("a0"),
                        }),
                        make_list({
                            make_atom("theorem"),
                            make_atom("a1"),
                        }),
                    }),
                },
            },
            {
                "redir [\"g\" Subgoal Subguide][sub Subgoal Subguide [theorem a1]]\n;",
                redir_statement{
                    .m_tag = make_list({
                        make_atom("g"),
                        make_var("Subgoal", l_var_alist),
                        make_var("Subguide", l_var_alist),
                    }),
                    .m_guide = make_list({
                        make_atom("sub"),
                        make_var("Subgoal", l_var_alist),
                        make_var("Subguide", l_var_alist),
                        make_list({
                            make_atom("theorem"),
                            make_atom("a1"),
                        }),
                    }),
                },

            },
            {
                "redir gt [mp [theorem a0] [theorem a1]];",
                redir_statement{
                    .m_tag = make_atom("gt"),
                    .m_guide = make_list({
                        make_atom("mp"),
                        make_list({
                            make_atom("theorem"),
                            make_atom("a0"),
                        }),
                        make_list({
                            make_atom("theorem"),
                            make_atom("a1"),
                        }),
                    }),
                },
            },
            {
                "redir [gt x z] [eval];",
                redir_statement{
                    .m_tag = make_list({
                        make_atom("gt"),
                        make_atom("x"),
                        make_atom("z"),
                    }),
                    .m_guide = make_list({
                        make_atom("eval"),
                    }),
                },
            },
            {
                "\'redir\' [\"t a g\" X Y| Rest] [\"theorem\" \'a0\'];",
                redir_statement{
                    .m_tag = make_list({
                                           make_atom("t a g"),
                                           make_var("X", l_var_alist),
                                           make_var("Y", l_var_alist),
                                       },
                                       make_var("Rest", l_var_alist)),
                    .m_guide = make_list({
                        make_atom("theorem"),
                        make_atom("a0"),
                    }),
                },
            },
            {
                "\"redir\" [\"X\" Args] [guide g0 Args];",
                redir_statement{
                    .m_tag = make_list({
                        make_atom("X"),
                        make_var("Args", l_var_alist),
                    }),
                    .m_guide = make_list({
                        make_atom("guide"),
                        make_atom("g0"),
                        make_var("Args", l_var_alist),
                    }),
                },
            },
        };

    for (const auto &[l_key, l_value] : l_test_cases)
    {
        fid_t l_case_frame = PL_open_foreign_frame();

        std::stringstream l_ss(l_key);

        statement l_statement;
        l_ss >> l_statement;

        // make sure the stringstream is not in failstate
        assert(!l_ss.fail());
        assert(std::holds_alternative<redir_statement>(l_statement));
        assert(l_statement == l_value);

        LOG("success, case: \"" << l_key << "\"" << std::endl);

        PL_discard_foreign_frame(l_case_frame);
    }

    std::vector<std::string> l_fail_cases =
        {
            "",
            "abc",
            "redir",
            "redir g0",
            "redir g0 []",
            "redir g0 [] [theorem a0]",
        };

    for (const auto &l_input : l_fail_cases)
    {
        fid_t l_case_frame = PL_open_foreign_frame();

        std::stringstream l_ss(l_input);

        statement l_statement;

        l_ss >> l_statement;

        // ensure failure of extraction
        assert(l_ss.fail());

        LOG("success, case: expected failure extracting redir_statement: " << l_input << std::endl);

        PL_discard_foreign_frame(l_case_frame);
    }

    PL_discard_foreign_frame(l_frame);
}

static void test_parser_extract_infer_statement()
{
    fid_t l_frame = PL_open_foreign_frame();

    constexpr bool ENABLE_DEBUG_LOGS = true;

    using unilog::infer_statement;
    using unilog::statement;

    std::map<std::string, term_t> l_var_alist;

    data_points<std::string, statement> l_test_cases =
        {
            {
                "infer i0 [bout daniel [mp [theorem a0] [theorem a1]]];",
                infer_statement{
                    .m_tag = make_atom("i0"),
                    .m_guide = make_list({
                        make_atom("bout"),
                        make_atom("daniel"),
                        make_list({
                            make_atom("mp"),
                            make_list({
                                make_atom("theorem"),
                                make_atom("a0"),
                            }),
                            make_list({
                                make_atom("theorem"),
                                make_atom("a1"),
                            }),
                        }),
                    }),
                },
            },
            {
                "infer 'i0' []\n;",
                infer_statement{
                    .m_tag = make_atom("i0"),
                    .m_guide = make_list({}),
                },
            },
            {
                "infer 'i0' [guide commute [guide g_add] [theorem not_add]]\n;",
                infer_statement{
                    .m_tag = make_atom("i0"),
                    .m_guide = make_list({
                        make_atom("guide"),
                        make_atom("commute"),
                        make_list({
                            make_atom("guide"),
                            make_atom("g_add"),
                        }),
                        make_list({
                            make_atom("theorem"),
                            make_atom("not_add"),
                        }),
                    }),
                },
            },
            {
                "infer X [guide g_add_ui];",
                infer_statement{
                    .m_tag = make_var("X", l_var_alist),
                    .m_guide = make_list({
                        make_atom("guide"),
                        make_atom("g_add_ui"),
                    }),
                },
            },
            {
                "\"infer\" [X|Y] [\"theorem\" awes];",
                infer_statement{
                    .m_tag = make_list({make_var("X", l_var_alist)}, make_var("Y", l_var_alist)),
                    .m_guide = make_list({
                        make_atom("theorem"),
                        make_atom("awes"),
                    }),
                },
            },
            {
                "\"infer\" [X] [\"theorem\" awes1];",
                infer_statement{
                    .m_tag = make_list({make_var("X", l_var_alist)}),
                    .m_guide = make_list({
                        make_atom("theorem"),
                        make_atom("awes1"),
                    }),
                },
            },
        };

    for (const auto &[l_key, l_value] : l_test_cases)
    {
        fid_t l_case_frame = PL_open_foreign_frame();

        std::stringstream l_ss(l_key);

        statement l_statement;

        l_ss >> l_statement;

        // make sure the stringstream is not in failstate
        assert(!l_ss.fail());
        assert(std::holds_alternative<infer_statement>(l_statement));
        assert(l_statement == l_value);

        LOG("success, case: \"" << l_key << "\"" << std::endl);

        PL_discard_foreign_frame(l_case_frame);
    }

    std::vector<std::string> l_fail_cases =
        {
            "",
            "a",
            "infer",
            "infer i0",
            "infer i0 theorem",
            "infer i0 theorem guide",
            "infer i0 theorem \';",
            "infer i0 theorem [;",
            "infer i0 theorem [;];",
            "infer i0 theorem [a | b | c];",
        };

    for (const auto &l_input : l_fail_cases)
    {
        fid_t l_case_frame = PL_open_foreign_frame();

        std::stringstream l_ss(l_input);

        statement l_statement;
        l_ss >> l_statement;

        // ensure failure of extraction
        assert(l_ss.fail());

        LOG("success, case: expected failure extracting redir_statement: " << l_input << std::endl);

        PL_discard_foreign_frame(l_case_frame);
    }

    PL_discard_foreign_frame(l_frame);
}

static void test_parser_extract_refer_statement()
{
    fid_t l_frame = PL_open_foreign_frame();

    constexpr bool ENABLE_DEBUG_LOGS = true;

    using unilog::atom;
    using unilog::refer_statement;
    using unilog::statement;

    std::map<std::string, term_t> l_var_alist;

    data_points<std::string, statement> l_test_cases =
        {
            {
                "refer leon \'./path/to/leon.u\';",
                refer_statement{
                    .m_tag = make_atom("leon"),
                    .m_file_path = make_atom("./path/to/leon.u"),
                },
            },
            {
                "refer [] \'./path/to/leon.u\';",
                refer_statement{
                    .m_tag = make_list({}),
                    .m_file_path = make_atom("./path/to/leon.u"),
                },
            },
            {
                "refer [abc] \'../module.u\'\n;",
                refer_statement{
                    .m_tag = make_list({make_atom("abc")}),
                    .m_file_path = make_atom("../module.u"),
                },
            },
            {
                "\"refer\" [X|Y] [list]\n;",
                refer_statement{
                    .m_tag = make_list({make_var("X", l_var_alist)}, make_var("Y", l_var_alist)),
                    .m_file_path = make_list({make_atom("list")}),
                },
            },
            {
                "\'refer\' [a b c | Y] Path\t;",
                refer_statement{
                    .m_tag = make_list({
                                           make_atom("a"),
                                           make_atom("b"),
                                           make_atom("c"),
                                       },
                                       make_var("Y", l_var_alist)),
                    .m_file_path = make_var("Path", l_var_alist),
                },
            },
        };

    for (const auto &[l_key, l_value] : l_test_cases)
    {
        fid_t l_case_frame = PL_open_foreign_frame();

        std::stringstream l_ss(l_key);

        statement l_statement;
        l_ss >> l_statement;

        // make sure the stringstream is not in failstate
        assert(!l_ss.fail());
        assert(std::holds_alternative<refer_statement>(l_statement));
        assert(l_statement == l_value);

        LOG("success, case: \"" << l_key << "\"" << std::endl);

        PL_discard_foreign_frame(l_case_frame);
    }

    std::vector<std::string> l_fail_cases =
        {
            "",
            "abc",
            "X",
            "refer r0",
            "refer r0 \'file/path\'",
            "refer r0 \';",
            "refer r0 \";",
            "refer r0 [;];",
        };

    for (const auto &l_input : l_fail_cases)
    {
        fid_t l_case_frame = PL_open_foreign_frame();

        std::stringstream l_ss(l_input);

        statement l_statement;
        l_ss >> l_statement;

        // ensure failure of extraction
        assert(l_ss.fail());

        LOG("success, case: expected failure extracting redir_statement: " << l_input << std::endl);

        PL_discard_foreign_frame(l_case_frame);
    }

    PL_discard_foreign_frame(l_frame);
}

static void test_parse_file_examples()
{
    constexpr bool ENABLE_DEBUG_LOGS = true;

    using unilog::atom;
    using unilog::axiom_statement;
    using unilog::infer_statement;
    using unilog::redir_statement;
    using unilog::refer_statement;
    using unilog::statement;

    std::map<std::string, term_t> l_var_alist;

    data_points<std::string, std::list<statement>> l_data_points =
        {
            {
                "./src/test_input_files/parser_example_0/main.ul",
                {
                    axiom_statement{
                        .m_tag = make_atom("a0"),
                        .m_theorem = make_atom("test"),
                    },
                    axiom_statement{
                        .m_tag = make_list({
                            make_atom("a1"),
                        }),
                        .m_theorem = make_atom("x"),
                    },
                    axiom_statement{
                        .m_tag = make_atom("a2"),
                        .m_theorem = make_list({
                            make_atom("x"),
                        }),
                    },
                },
            },
            {
                "./src/test_input_files/parser_example_1/main.ul",
                {
                    axiom_statement{
                        .m_tag = make_atom("a0"),
                        .m_theorem = make_list({
                            make_atom("if"),
                            make_atom("y"),
                            make_atom("x"),
                        }),
                    },
                    axiom_statement{
                        .m_tag = make_atom("a1"),
                        .m_theorem = make_atom("x"),
                    },
                },
            },
            {
                "./src/test_input_files/parser_example_2/main.u",
                {
                    axiom_statement{
                        .m_tag = make_list({
                                               make_atom("a0"),
                                               make_var("X", l_var_alist),
                                           },
                                           make_var("X", l_var_alist)),
                        .m_theorem = make_list({
                            make_atom("claim"),
                            make_atom("daniel"),
                            make_list({
                                make_atom("if"),
                                make_atom("y"),
                                make_atom("x"),
                            }),
                        }),
                    },
                    axiom_statement{
                        .m_tag = make_list({
                                               make_atom("a1"),
                                               make_var("Y", l_var_alist),
                                           },
                                           make_var("Z", l_var_alist)),
                        .m_theorem = make_list({
                            make_atom("claim"),
                            make_atom("daniel"),
                            make_atom("x"),
                        }),
                    },
                    infer_statement{
                        .m_tag = make_atom("i0"),
                        .m_guide = make_list({
                            make_atom("bout"),
                            make_atom("daniel"),
                            make_list({
                                make_atom("mp"),
                                make_list({
                                    make_atom("theorem"),
                                    make_list({
                                                  make_atom("a0"),
                                                  make_var("A", l_var_alist),
                                              },
                                              make_var("B", l_var_alist)),
                                }),
                                make_list({
                                    make_atom("theorem"),
                                    make_list({
                                                  make_atom("a1"),
                                                  make_var("A", l_var_alist),
                                              },
                                              make_var("B", l_var_alist)),
                                }),
                            }),
                        }),
                    },
                },
            },
            {
                "./src/test_input_files/parser_example_3/main.u",
                {
                    axiom_statement{
                        .m_tag = make_list({
                                               make_atom("a0\n"),
                                               make_var("X", l_var_alist),
                                           },
                                           make_var("X", l_var_alist)),
                        .m_theorem = make_list({
                            make_atom("claim"),
                            make_atom("daniel"),
                            make_list({
                                make_atom("if"),
                                make_atom("y"),
                                make_atom("x"),
                            }),
                        }),
                    },
                    axiom_statement{
                        .m_tag = make_list({
                                               make_atom("a1"),
                                               make_var("Y", l_var_alist),
                                           },
                                           make_var("Z", l_var_alist)),
                        .m_theorem = make_list({
                            make_atom("claim"),
                            make_atom("daniel"),
                            make_atom("x"),
                        }),
                    },
                    redir_statement{
                        .m_tag = make_list({
                            make_atom("g0"),
                        }),
                        .m_guide = make_list({
                            make_atom("bout"),
                            make_atom("daniel"),
                            make_list({
                                make_atom("mp"),
                                make_list({
                                    make_atom("theorem"),
                                    make_list({
                                                  make_atom("a0"),
                                                  make_var("A", l_var_alist),
                                              },
                                              make_var("B", l_var_alist)),
                                }),
                                make_list({
                                    make_atom("theorem"),
                                    make_list({
                                                  make_atom("a1"),
                                                  make_var("A", l_var_alist),
                                              },
                                              make_var("B", l_var_alist)),
                                }),
                            }),
                        }),
                    },
                    infer_statement{
                        .m_tag = make_atom("i0"),
                        .m_guide = make_list({
                            make_atom("guide"),
                            make_list({
                                make_atom("g0"),
                            }),
                        }),
                    },
                },
            },
            {
                "./src/test_input_files/parser_example_4/main.u",
                {
                    refer_statement{
                        .m_tag = make_list({
                                               make_list({
                                                   make_list({
                                                       make_atom("abc"),
                                                   }),
                                                   make_var("X", l_var_alist),
                                               }),
                                           },
                                           make_var("X", l_var_alist)),
                        .m_file_path = make_atom("path to file"),
                    },
                    axiom_statement{
                        .m_tag = make_var("A", l_var_alist),
                        .m_theorem = make_list({
                            make_atom("claim"),
                            make_atom("daniel"),
                            make_var("A", l_var_alist),
                        }),
                    },
                },
            },
            {
                "./src/test_input_files/parser_example_5/jake.u",
                {
                    axiom_statement{
                        .m_tag = make_atom("a0"),
                        .m_theorem = make_list({
                            make_atom("if"),
                            make_atom("y"),
                            make_atom("x"),
                        }),
                    },
                    axiom_statement{
                        .m_tag = make_atom("a1"),
                        .m_theorem = make_atom("x"),
                    },
                    redir_statement{
                        .m_tag = make_atom("g0"),
                        .m_guide = make_list({
                            make_atom("mp"),
                            make_list({
                                make_atom("theorem"),
                                make_atom("a0"),
                            }),
                            make_list({
                                make_atom("theorem"),
                                make_atom("a1"),
                            }),
                        }),
                    },
                    infer_statement{
                        .m_tag = make_atom("i0"),
                        .m_guide = make_list({
                            make_atom("guide"),
                            make_atom("g0"),
                        }),
                    },
                    refer_statement{
                        .m_tag = make_atom("r0"),
                        .m_file_path = make_atom("abc 123"),
                    },
                },
            },
        };

    for (const auto &[l_file_path, l_expected] : l_data_points)
    {
        std::ifstream l_if(l_file_path);

        std::list<statement> l_statements;
        std::copy(std::istream_iterator<statement>(l_if), std::istream_iterator<statement>(), std::back_inserter(l_statements));

        assert(l_statements == l_expected);

        LOG("success, parsed file: " << l_file_path << std::endl);
    }
}

void test_parser_main()
{
    constexpr bool ENABLE_DEBUG_LOGS = true;

    TEST(test_make_nil);
    TEST(test_make_atom);
    TEST(test_make_list);
    TEST(test_make_variable);
    TEST(test_random_string);
    TEST(test_equal_forms);
    TEST(test_axiom_statement_equal);
    TEST(test_redir_statement_equal);
    TEST(test_infer_statement_equal);
    TEST(test_refer_statement_equal);
    TEST(test_parser_extract_prolog_expression);
    TEST(test_parser_extract_axiom_statement);
    TEST(test_parser_extract_redir_statement);
    TEST(test_parser_extract_infer_statement);
    TEST(test_parser_extract_refer_statement);
    TEST(test_parse_file_examples);
}

#endif
