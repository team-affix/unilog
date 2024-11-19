#include <iterator>
#include <algorithm>
#include <map>

#include "parser.hpp"
#include "lexer.hpp"

/// This macro function defines
///     getting a value from cache if key contained,
///     otherwise, computing value and caching it.
#define CACHE(cache, key, value) \
    (cache.contains(key) ? cache[key] : cache[key] = value)

namespace unilog
{

    std::istream &extract_term_t(std::istream &a_istream, term_t a_term_t, std::map<std::string, term_t> &a_var_alist)
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
            {
                a_istream.setstate(std::ios::failbit);
                return a_istream;
            }
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
            auto l_search_result = a_var_alist.find(l_variable.m_identifier);

            /////////////////////////////////////////
            // no entry exists
            /////////////////////////////////////////
            if (l_search_result == a_var_alist.end())
            {
                // simply add the entry for this variable.
                a_var_alist[l_variable.m_identifier] = a_term_t;
                return a_istream;
            }

            /////////////////////////////////////////
            // entry already exists;
            /////////////////////////////////////////
            if (!PL_unify(a_term_t, l_search_result->second))
            {
                a_istream.setstate(std::ios::failbit);
                return a_istream;
            }
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

            /////////////////////////////////////////
            // terminator will either be a list_close ']' or list_separator '|'
            /////////////////////////////////////////
            lexeme l_list_terminator;
            a_istream >> l_list_terminator;

            if (a_istream.fail())
            {
                a_istream.setstate(std::ios::failbit);
                return a_istream;
            }

            /////////////////////////////////////////
            // set the list's tail (into a_term_t)
            /////////////////////////////////////////
            if (std::holds_alternative<list_close>(l_list_terminator))
            {
                /////////////////////////////////////////
                // default tail is always nil
                /////////////////////////////////////////
                if (!PL_put_nil(a_term_t))
                {
                    a_istream.setstate(std::ios::failbit);
                    return a_istream;
                }
            }
            else if (std::holds_alternative<list_separator>(l_list_terminator))
            {
                /////////////////////////////////////////
                // extract the tail of the list.
                // could be var or atom, or any expr
                /////////////////////////////////////////
                extract_term_t(a_istream, a_term_t, a_var_alist);

                /////////////////////////////////////////
                // finally, extract the list_close
                /////////////////////////////////////////
                lexeme l_list_close;
                a_istream >> l_list_close;

                if (a_istream.fail() || !std::holds_alternative<list_close>(l_list_close))
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

            /////////////////////////////////////////
            // build list in reverse order
            /////////////////////////////////////////
            for (auto l_it = l_list.rbegin(); l_it != l_list.rend(); l_it++)
            {
                if (!PL_cons_list(a_term_t, *l_it, a_term_t))
                {
                    a_istream.setstate(std::ios::failbit);
                    return a_istream;
                }
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
        /////////////////////////////////////////
        // extract the command lexeme
        /////////////////////////////////////////
        lexeme l_command;
        a_istream >> l_command;

        // atom is expected for the command
        if (!std::holds_alternative<atom>(l_command))
        {
            a_istream.setstate(std::ios::failbit);
            return a_istream;
        }

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

            extract_term_t(a_istream, l_result.m_tag, l_var_alist);
            extract_term_t(a_istream, l_result.m_theorem, l_var_alist);

            a_statement = l_result;
        }
        else if (l_command_text == "guide")
        {
            guide_statement l_result;

            /////////////////////////////////////////
            // creates new term refs
            /////////////////////////////////////////
            l_result.m_tag = PL_new_term_ref();
            l_result.m_args = PL_new_term_ref();
            l_result.m_guide = PL_new_term_ref();

            extract_term_t(a_istream, l_result.m_tag, l_var_alist);
            extract_term_t(a_istream, l_result.m_args, l_var_alist);
            extract_term_t(a_istream, l_result.m_guide, l_var_alist);

            a_statement = l_result;
        }
        else if (l_command_text == "infer")
        {
            infer_statement l_result;

            /////////////////////////////////////////
            // creates new term refs
            /////////////////////////////////////////
            l_result.m_tag = PL_new_term_ref();
            l_result.m_theorem = PL_new_term_ref();
            l_result.m_guide = PL_new_term_ref();

            extract_term_t(a_istream, l_result.m_tag, l_var_alist);
            extract_term_t(a_istream, l_result.m_theorem, l_var_alist);
            extract_term_t(a_istream, l_result.m_guide, l_var_alist);

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

#ifdef UNIT_TEST

#include <fstream>
#include <iterator>
#include "test_utils.hpp"

////////////////////////////////
//// HELPER FUNCTIONS
////////////////////////////////

static term_t make_nil()
{
    term_t l_result = PL_new_term_ref();
    assert(PL_put_nil(l_result));
    return l_result;
}

static term_t make_atom(const std::string &a_text)
{
    term_t l_result = PL_new_term_ref();
    assert(PL_put_atom_chars(l_result, a_text.c_str()));
    return l_result;
}

static term_t make_list(const std::list<term_t> &a_elements, term_t a_tail = make_nil())
{
    term_t l_result = a_tail;

    for (auto l_it = a_elements.rbegin(); l_it != a_elements.rend(); l_it++)
    {
        // just toss it into an assertion, since we want it to always succeed.
        assert(PL_cons_list(l_result, *l_it, l_result));
    }

    return l_result;
}

static term_t make_var(const std::string &a_identifier, std::map<std::string, term_t> &a_var_alist)
{
    if (a_identifier == "_")
        return PL_new_term_ref(); // singleton always gets own term.
    return CACHE(a_var_alist, a_identifier, PL_new_term_ref());
}

////////////////////////////////
//// TESTS
////////////////////////////////

static void test_make_nil()
{
    fid_t l_frame_id = PL_open_foreign_frame();

    term_t l_nil = make_nil();

    assert(PL_get_nil(l_nil));

    PL_close_foreign_frame(l_frame_id);
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

    PL_close_foreign_frame(l_frame_id);
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

    PL_close_foreign_frame(l_frame_id);
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

    PL_close_foreign_frame(l_frame_id);
}

static void test_parser_extract_prolog_expression()
{
    constexpr bool ENABLE_DEBUG_LOGS = true;

    std::map<std::string, term_t> l_var_alist;

    data_points<std::string, std::function<bool(term_t)>>
        l_test_cases =
            {
                {
                    "if",
                    [](term_t a_term)
                    {
                        return PL_compare(a_term, make_atom("if")) == 0;
                    },
                },
                {
                    "add",
                    [](term_t a_term)
                    {
                        return PL_compare(a_term, make_atom("add")) == 0;
                    },
                },
                {
                    "\'\'",
                    [](term_t a_term)
                    {
                        return PL_compare(a_term, make_atom("")) == 0;
                    },
                },
                // {
                //     "Var",
                //     make_var("Var", l_var_alist),
                // },
                // {
                //     "_",
                //     make_var("_", l_var_alist),
                // },
                // {
                //     // Test singular extraction not perturbed by later content
                //     "_ abc",
                //     make_var("_", l_var_alist),
                // },
                // {
                //     "[]",
                //     make_list({}),
                // },
                // {
                //     "[] []",
                //     make_list({}),
                // },
                // {
                //     "[abc]",
                //     make_list({
                //         make_atom("abc"),
                //     }),
                // },
                // {
                //     "[abc Var]",
                //     make_list({
                //         make_atom("abc"),
                //         make_var("Var", l_var_alist),
                //     }),
                // },
                // {
                //     "[abc Var def]",
                //     make_list({
                //         make_atom("abc"),
                //         make_var("Var", l_var_alist),
                //         make_atom("def"),
                //     }),
                // },
                // {
                //     "[[]]",
                //     make_list({
                //         make_list({}),
                //     }),
                // },
                // {
                //     "[abc []]",
                //     make_list({
                //         make_atom("abc"),
                //         make_list({}),
                //     }),
                // },
                // {
                //     "[abc [] def]",
                //     make_list({
                //         make_atom("abc"),
                //         make_list({}),
                //         make_atom("def"),
                //     }),
                // },
                // {
                //     "[abc [123] def]",
                //     make_list({
                //         make_atom("abc"),
                //         make_list({
                //             make_atom("123"),
                //         }),
                //         make_atom("def"),
                //     }),
                // },
                // {
                //     "abc []",
                //     make_atom("abc"),
                // },
                // {
                //     "[abc [123 [def [456 [ghi]]]]]",
                //     make_list({
                //         make_atom("abc"),
                //         make_list({
                //             make_atom("123"),
                //             make_list({
                //                 make_atom("def"),
                //                 make_list({
                //                     make_atom("456"),
                //                     make_list({
                //                         make_atom("ghi"),
                //                     }),
                //                 }),
                //             }),
                //         }),
                //     }),
                // },
                // {
                //     "[abc [_ _] [def A]]",
                //     make_list({
                //         make_atom("abc"),
                //         make_list({
                //             make_var("_", l_var_alist),
                //             make_var("_", l_var_alist),
                //         }),
                //         make_list({
                //             make_atom("def"),
                //             make_var("A", l_var_alist),
                //         }),
                //     }),
                // },
                // {
                //     // Single atom, with special characters
                //     "test!@.$^&*()",
                //     make_atom("test!@.$^&*()"),
                // },
                // {
                //     // single atom, it will not fail even with the closing paren. this is because,
                //     //     closing paren is a lexeme separator char and thus it will only extract the
                //     //     first prolog expression, "abc"
                //     "abc]",
                //     make_atom("abc"),
                // },
                // {
                //     "[123 456 7.89]",
                //     make_list({
                //         make_atom("123"),
                //         make_atom("456"),
                //         make_atom("7.89"),
                //     }),
                // },
                // {
                //     "[123 456 7.89 A [_ _]]",
                //     make_list({
                //         make_atom("123"),
                //         make_atom("456"),
                //         make_atom("7.89"),
                //         make_var("A", l_var_alist),
                //         make_list({
                //             make_var("_", l_var_alist),
                //             make_var("_", l_var_alist),
                //         }),
                //     }),
                // },
                // {
                //     // Having fun, realistic scenario
                //     "[if\n"
                //     "    [add X Y Z]\n"
                //     "    [and\n"
                //     "        [cons X XH XT]\n"
                //     "        [cons Y YH YT]\n"
                //     "        [add [XH] [YH] [ZH]]\n"
                //     "        [add XT YT ZT]\n"
                //     "        [cons Z ZH ZT]\n"
                //     "    ]\n"
                //     "]\n",
                //     make_list({
                //         make_atom("if"),
                //         make_list({
                //             make_atom("add"),
                //             make_var("X", l_var_alist),
                //             make_var("Y", l_var_alist),
                //             make_var("Z", l_var_alist),
                //         }),
                //         make_list({
                //             make_atom("and"),
                //             make_list({
                //                 make_atom("cons"),
                //                 make_var("X", l_var_alist),
                //                 make_var("XH", l_var_alist),
                //                 make_var("XT", l_var_alist),
                //             }),
                //             make_list({
                //                 make_atom("cons"),
                //                 make_var("Y", l_var_alist),
                //                 make_var("YH", l_var_alist),
                //                 make_var("YT", l_var_alist),
                //             }),
                //             make_list({
                //                 make_atom("add"),
                //                 make_list({make_var("XH", l_var_alist)}),
                //                 make_list({make_var("YH", l_var_alist)}),
                //                 make_list({make_var("ZH", l_var_alist)}),
                //             }),
                //             make_list({
                //                 make_atom("add"),
                //                 make_var("XT", l_var_alist),
                //                 make_var("YT", l_var_alist),
                //                 make_var("ZT", l_var_alist),
                //             }),
                //             make_list({
                //                 make_atom("cons"),
                //                 make_var("Z", l_var_alist),
                //                 make_var("ZH", l_var_alist),
                //                 make_var("ZT", l_var_alist),
                //             }),
                //         }),
                //     }),
                // },
                // {
                //     "[[[] abc] 123]",
                //     make_list({
                //         make_list({
                //             make_list({

                //             }),
                //             make_atom("abc"),
                //         }),
                //         make_atom("123"),
                //     }),
                // },
            };

    for (const auto &[l_key, l_value] : l_test_cases)
    {
        std::stringstream l_ss(l_key);

        term_t l_exp = PL_new_term_ref();

        unilog::extract_term_t(l_ss, l_exp, l_var_alist);

        // open PL stack frame
        fid_t l_frame_id = PL_open_foreign_frame();

        assert(l_value(l_exp));

        PL_close_foreign_frame(l_frame_id);

        assert(!l_ss.fail());

        LOG("success, case: \"" << l_key << "\"" << std::endl);
    }

    // std::vector<std::string> l_expect_failure_inputs =
    //     {
    //         "[abc",
    //         "[[abc] [123]",
    //         "]",
    //         "\'",
    //         "\"",
    //         // "abc]", // this is NOT an expect failure input.
    //         // this is because it will only try to parse the first prolog expression before a lexeme separator char.

    //     };

    // for (const auto &l_input : l_expect_failure_inputs)
    // {
    //     std::stringstream l_ss(l_input);

    //     term_t l_exp;

    //     unilog::extract_term_t(l_ss, l_exp, l_var_alist);

    //     // make sure the extraction was unsuccessful
    //     assert(l_ss.fail());

    //     LOG("success, expected throw, case: " << l_input << std::endl);
    // }
}

// void test_parser_extract_axiom_statement()
// {
//     fid_t l_frame_id = PL_open_foreign_frame();

//     constexpr bool ENABLE_DEBUG_LOGS = true;

//     using unilog::axiom_statement;
//     using unilog::statement;

//     std::map<std::string, term_t> l_var_alist;

//     data_points<std::string, axiom_statement> l_test_cases =
//         {
//             {
//                 "axiom a0 x",
//                 axiom_statement{
//                     .m_tag =
//                         make_atom("a0"),
//                     .m_theorem =
//                         make_atom("x"),
//                 },
//             },
//             {
//                 "axiom add_bc_0 [add [] L L]",
//                 axiom_statement{
//                     .m_tag =
//                         make_atom("add_bc_0"),
//                     .m_theorem =
//                         make_list({
//                             make_atom("add"),
//                             make_list({}),
//                             make_var("L", l_var_alist),
//                             make_var("L", l_var_alist),
//                         }),
//                 },
//             },
//             {
//                 "axiom @tag [awesome X]",
//                 axiom_statement{
//                     .m_tag =
//                         make_atom("@tag"),
//                     .m_theorem =
//                         make_list({
//                             make_atom("awesome"),
//                             make_var("X", l_var_alist),
//                         }),
//                 },
//             },
//             {
//                 "axiom tAg123@#$%^&*() _",
//                 axiom_statement{
//                     .m_tag =
//                         make_atom("tAg123@#$%^&*()"),
//                     .m_theorem =
//                         make_var("_", l_var_alist),
//                 },
//             },
//             {
//                 "axiom tag[theorem]",
//                 axiom_statement{
//                     .m_tag =
//                         make_atom("tag"),
//                     .m_theorem =
//                         make_list({
//                             make_atom("theorem"),
//                         }),
//                 },
//             },
//             {
//                 "axiom abc 123",
//                 axiom_statement{
//                     .m_tag =
//                         make_atom("abc"),
//                     .m_theorem =
//                         make_atom("123"),
//                 },
//             },
//             {
//                 "axiom 123 [[[]] a 123]",
//                 axiom_statement{
//                     .m_tag =
//                         make_atom("123"),
//                     .m_theorem =
//                         make_list({
//                             make_list({
//                                 make_list({

//                                 }),
//                             }),
//                             make_atom("a"),
//                             make_atom("123"),
//                         }),
//                 },
//             },
//             {
//                 "axiom +/bc/0\n"
//                 "[+\n"
//                 "    []\n"
//                 "    L\n"
//                 "    L\n"
//                 "]",
//                 axiom_statement{
//                     .m_tag =
//                         make_atom("+/bc/0"),
//                     .m_theorem =
//                         make_list({
//                             make_atom("+"),
//                             make_list({}),
//                             make_var("L", l_var_alist),
//                             make_var("L", l_var_alist),
//                         }),
//                 },
//             },
//             {
//                 "axiom 123[\'! this is a \\t quotation\']",
//                 axiom_statement{
//                     .m_tag =
//                         make_atom("123"),
//                     .m_theorem =
//                         make_list({
//                             make_atom("! this is a \t quotation"),
//                         }),
//                 },
//             },
//             {
//                 "axiom \'tag\' theorem",
//                 axiom_statement{
//                     .m_tag =
//                         make_atom("tag"),
//                     .m_theorem =
//                         make_atom("theorem"),
//                 },
//             },
//             {
//                 "axiom \"tag\" theorem",
//                 axiom_statement{
//                     .m_tag =
//                         make_atom("tag"),
//                     .m_theorem =
//                         make_atom("theorem"),
//                 },
//             },
//         };

//     for (const auto &[l_key, l_value] : l_test_cases)
//     {
//         std::stringstream l_ss(l_key);

//         statement l_exp;
//         l_ss >> l_exp;

//         axiom_statement l_axiom_statement = std::get<axiom_statement>(l_exp);

//         assert(PL_unify(l_axiom_statement.m_tag, l_value.m_tag));
//         assert(PL_unify(l_axiom_statement.m_theorem, l_value.m_theorem));

//         // make sure the stringstream is not in failstate
//         assert(!l_ss.fail());

//         LOG("success, case: \"" << l_key << "\"" << std::endl);
//     }

//     std::vector<std::string> l_fail_cases =
//         {
//             "",
//             "abc",
//             "_ _",
//             "VariableTag Theorem",
//             "VariableTag atom",
//             "VariableTag [elem0 elem1]",
//             "[] theorem",
//             "[X] theorem",
//             "[atom] theorem",
//             "axiom a0",
//             "axiom \'a0\'",
//             "axiom [tag] [expr]",
//             "guide a0 x",
//             "infer i0 x",
//             "refer r0 x",
//         };

//     for (const auto &l_input : l_fail_cases)
//     {
//         std::stringstream l_ss(l_input);

//         axiom_statement l_axiom_statement;

//         l_ss >> l_axiom_statement;

//         // ensure failure of extraction
//         assert(l_ss.fail());

//         LOG("success, case: expected failure extracting axiom_statement: " << l_input << std::endl);
//     }

//     PL_close_foreign_frame(l_frame_id);
// }

// void test_parser_extract_guide_statement()
// {
//     constexpr bool ENABLE_DEBUG_LOGS = true;

//     using unilog::atom;
//     using unilog::guide_statement;
//     using unilog::lexeme;
//     using unilog::list_close;
//     using unilog::list_open;
//     using unilog::quoted_atom;
//     using unilog::term;
//     using unilog::unquoted_atom;
//     using unilog::variable;

//     data_points<std::string, guide_statement> l_test_cases =
//         {
//             {
//                 "guide g_add_bc []\n"
//                 "[gor\n"
//                 "    add_bc_0\n"
//                 "    add_bc_1\n"
//                 "    add_bc_2\n"
//                 "    add_bc_3\n"
//                 "    add_bc_4\n"
//                 "    add_bc_5\n"
//                 "]\n",
//                 guide_statement{
//                     .m_tag =
//                         term{
//                             unquoted_atom{
//                                 "g_add_bc",
//                             },
//                         },
//                     .m_args = std::list<variable>({}),
//                     .m_guide =
//                         term{
//                             std::list<term>{
//                                 term{
//                                     unquoted_atom{
//                                         "gor",
//                                     },
//                                 },
//                                 term{
//                                     unquoted_atom{
//                                         "add_bc_0",
//                                     },
//                                 },
//                                 term{
//                                     unquoted_atom{
//                                         "add_bc_1",
//                                     },
//                                 },
//                                 term{
//                                     unquoted_atom{
//                                         "add_bc_2",
//                                     },
//                                 },
//                                 term{
//                                     unquoted_atom{
//                                         "add_bc_3",
//                                     },
//                                 },
//                                 term{
//                                     unquoted_atom{
//                                         "add_bc_4",
//                                     },
//                                 },
//                                 term{
//                                     unquoted_atom{
//                                         "add_bc_5",
//                                     },
//                                 },
//                             },
//                         },
//                 },
//             },
//             {
//                 "guide g0[][bind K [theorem a0]]",
//                 guide_statement{
//                     .m_tag = unquoted_atom{"g0"},
//                     .m_args = std::list<variable>({}),
//                     .m_guide = term{std::list<term>({
//                         term{
//                             unquoted_atom{"bind"},
//                         },
//                         term{
//                             variable{"K"},
//                         },
//                         term{std::list<term>({
//                             term{
//                                 unquoted_atom{"theorem"},
//                             },
//                             term{
//                                 unquoted_atom{"a0"},
//                             },
//                         })},
//                     })}},
//             },
//             {"guide \"g\"[] [sub thm [theorem a0] [theorem a1]]",
//              guide_statement{
//                  .m_tag = quoted_atom{"g"},
//                  .m_args = std::list<variable>({}),
//                  .m_guide = term{
//                      std::list<term>({
//                          term{unquoted_atom{"sub"}},
//                          term{unquoted_atom{"thm"}},
//                          term{std::list<term>({
//                              term{unquoted_atom{"theorem"}},
//                              term{unquoted_atom{"a0"}},
//                          })},
//                          term{std::list<term>({
//                              term{unquoted_atom{"theorem"}},
//                              term{unquoted_atom{"a1"}},
//                          })},
//                      })}}},
//             {
//                 "guide \"g\" [Subgoal Subguide][sub Subgoal Subguide [theorem a1]]",
//                 guide_statement{
//                     .m_tag = quoted_atom{"g"},
//                     .m_args = std::list<variable>({
//                         variable{"Subgoal"},
//                         variable{"Subguide"},
//                     }),
//                     .m_guide = term{std::list<term>({
//                         term{
//                             unquoted_atom{"sub"},
//                         },
//                         term{
//                             variable{"Subgoal"},
//                         },
//                         term{
//                             variable{"Subguide"},
//                         },
//                         term{std::list<term>({
//                             term{
//                                 unquoted_atom{"theorem"},
//                             },
//                             term{
//                                 unquoted_atom{"a1"},
//                             },
//                         })},
//                     })},
//                 },
//             },
//             {
//                 "guide gt [] [mp [theorem a0] [theorem a1]]",
//                 guide_statement{
//                     .m_tag = unquoted_atom{"gt"},
//                     .m_args = std::list<variable>({}),
//                     .m_guide = term{
//                         std::list<term>({
//                             term{
//                                 unquoted_atom{"mp"},
//                             },
//                             term{
//                                 std::list<term>({
//                                     term{
//                                         unquoted_atom{"theorem"},
//                                     },
//                                     term{
//                                         unquoted_atom{"a0"},
//                                     },
//                                 }),
//                             },
//                             term{
//                                 std::list<term>({
//                                     term{
//                                         unquoted_atom{"theorem"},
//                                     },
//                                     term{
//                                         unquoted_atom{"a1"},
//                                     },
//                                 }),
//                             },
//                         }),
//                     },
//                 },
//             },
//         };

//     for (const auto &[l_key, l_value] : l_test_cases)
//     {
//         std::stringstream l_ss(l_key);

//         guide_statement l_exp;

//         l_ss >> l_exp;

//         assert(l_exp == l_value);

//         // make sure the stringstream is not in failstate
//         assert(!l_ss.fail());

//         LOG("success, case: \"" << l_key << "\"" << std::endl);
//     }

//     std::vector<std::string> l_fail_cases =
//         {
//             "_",
//             "abc",
//             "",
//             "_ _",
//             "VariableTag Guide",
//             "VariableTag atom",
//             "VariableTag [elem0 elem1]",
//             "[] theorem",
//             "[X] theorem",
//             "[atom] theorem",
//             "axiom a0",
//             "axiom \'a0\'",
//             "axiom [tag] [expr]",
//             "axiom a0 x"
//             "infer i0 x",
//             "refer r0 x",
//             "guide g0 [",
//             "guide g0 [test] [redir]",   // arg list is not comprised of variables
//             "guide g0 [V test] [redir]", // arg list is not comprised of only variables
//             "guide g0 n [theorem a0]",
//         };

//     for (const auto &l_input : l_fail_cases)
//     {
//         std::stringstream l_ss(l_input);

//         guide_statement l_statement;

//         l_ss >> l_statement;

//         // ensure failure of extraction
//         assert(l_ss.fail());

//         LOG("success, case: expected failure extracting guide_statement: " << l_input << std::endl);
//     }
// }

// void test_parser_extract_infer_statement()
// {
//     constexpr bool ENABLE_DEBUG_LOGS = true;

//     using unilog::atom;
//     using unilog::infer_statement;
//     using unilog::lexeme;
//     using unilog::list_close;
//     using unilog::list_open;
//     using unilog::quoted_atom;
//     using unilog::term;
//     using unilog::unquoted_atom;
//     using unilog::variable;

//     data_points<std::string, infer_statement> l_test_cases =
//         {
//             {
//                 "infer i0 [claim daniel y] [bout daniel [mp [theorem a0] [theorem a1]]]",
//                 infer_statement{
//                     .m_tag = unquoted_atom{"i0"},
//                     .m_theorem = term{std::list<term>({
//                         term{unquoted_atom{"claim"}},
//                         term{unquoted_atom{"daniel"}},
//                         term{unquoted_atom{"y"}},
//                     })},
//                     .m_guide = term{std::list<term>({
//                         term{unquoted_atom{"bout"}},
//                         term{unquoted_atom{"daniel"}},
//                         term{std::list<term>({
//                             term{unquoted_atom{"mp"}},
//                             term{std::list<term>({
//                                 term{unquoted_atom{"theorem"}},
//                                 term{unquoted_atom{"a0"}},
//                             })},
//                             term{std::list<term>({
//                                 term{unquoted_atom{"theorem"}},
//                                 term{unquoted_atom{"a1"}},
//                             })},
//                         })},
//                     })},
//                 },
//             },
//             {
//                 "infer 'i0' [claim daniel y] [bout daniel [mp [theorem a0] [theorem a1]]]",
//                 infer_statement{
//                     .m_tag = quoted_atom{"i0"},
//                     .m_theorem = term{std::list<term>({
//                         term{unquoted_atom{"claim"}},
//                         term{unquoted_atom{"daniel"}},
//                         term{unquoted_atom{"y"}},
//                     })},
//                     .m_guide = term{std::list<term>({
//                         term{unquoted_atom{"bout"}},
//                         term{unquoted_atom{"daniel"}},
//                         term{std::list<term>({
//                             term{unquoted_atom{"mp"}},
//                             term{std::list<term>({
//                                 term{unquoted_atom{"theorem"}},
//                                 term{unquoted_atom{"a0"}},
//                             })},
//                             term{std::list<term>({
//                                 term{unquoted_atom{"theorem"}},
//                                 term{unquoted_atom{"a1"}},
//                             })},
//                         })},
//                     })},
//                 },
//             },
//             {
//                 "infer 'i0' [claim daniel y] [bout daniel [dout daniel [mp [theorem a2] [theorem a3]]]]",
//                 infer_statement{
//                     .m_tag = quoted_atom{"i0"},
//                     .m_theorem = term{std::list<term>({
//                         term{unquoted_atom{"claim"}},
//                         term{unquoted_atom{"daniel"}},
//                         term{unquoted_atom{"y"}},
//                     })},
//                     .m_guide = term{std::list<term>({
//                         term{unquoted_atom{"bout"}},
//                         term{unquoted_atom{"daniel"}},
//                         term{std::list<term>({
//                             term{unquoted_atom{"dout"}},
//                             term{unquoted_atom{"daniel"}},
//                             term{std::list<term>({
//                                 term{unquoted_atom{"mp"}},
//                                 term{std::list<term>({
//                                     term{unquoted_atom{"theorem"}},
//                                     term{unquoted_atom{"a2"}},
//                                 })},
//                                 term{std::list<term>({
//                                     term{unquoted_atom{"theorem"}},
//                                     term{unquoted_atom{"a3"}},
//                                 })},
//                             })},
//                         })},
//                     })},
//                 },
//             },
//             {
//                 "infer 'i0' [claim daniel y] a0",
//                 infer_statement{
//                     .m_tag = quoted_atom{"i0"},
//                     .m_theorem = term{std::list<term>({
//                         term{unquoted_atom{"claim"}},
//                         term{unquoted_atom{"daniel"}},
//                         term{unquoted_atom{"y"}},
//                     })},
//                     .m_guide = term{unquoted_atom{"a0"}},
//                 },
//             },
//         };

//     for (const auto &[l_key, l_value] : l_test_cases)
//     {
//         std::stringstream l_ss(l_key);

//         infer_statement l_exp;

//         l_ss >> l_exp;

//         assert(l_exp == l_value);

//         // make sure the stringstream is not in failstate
//         assert(!l_ss.fail());

//         LOG("success, case: \"" << l_key << "\"" << std::endl);
//     }

//     std::vector<std::string> l_fail_cases =
//         {
//             "_",
//             "abc",
//             "",
//             "_ _",
//             "VariableTag Guide",
//             "VariableTag atom",
//             "VariableTag [elem0 elem1]",
//             "[] theorem",
//             "[X] theorem",
//             "[atom] theorem",
//             "axiom a0",
//             "axiom \'a0\'",
//             "axiom [tag] [expr]",
//             "axiom tag x",
//             "axiom a0 x"
//             "infer i0 x",
//             "infer [i0] x [theorem a0]", // tag is not atomic
//             "refer r0 x",
//             "guide g0 [",
//             "guide g0 [test] [redir]",
//             "guide g0 [V test] [redir]",
//             "guide g0 n [theorem a0]",
//             "guide g0 [] [theorem a0]",
//             "infer [",
//             "infer ]",
//             "infer \'i0",
//         };

//     for (const auto &l_input : l_fail_cases)
//     {
//         std::stringstream l_ss(l_input);

//         infer_statement l_statement;

//         l_ss >> l_statement;

//         // ensure failure of extraction
//         assert(l_ss.fail());

//         LOG("success, case: expected failure extracting guide_statement: " << l_input << std::endl);
//     }
// }

// void test_parser_extract_refer_statement()
// {
//     constexpr bool ENABLE_DEBUG_LOGS = true;

//     using unilog::atom;
//     using unilog::lexeme;
//     using unilog::list_close;
//     using unilog::list_open;
//     using unilog::quoted_atom;
//     using unilog::refer_statement;
//     using unilog::term;
//     using unilog::unquoted_atom;
//     using unilog::variable;

//     data_points<std::string, refer_statement> l_test_cases =
//         {
//             {
//                 "refer leon \'./path/to/leon.u\'",
//                 refer_statement{
//                     .m_tag = unquoted_atom{"leon"},
//                     .m_file_path = quoted_atom{"./path/to/leon.u"},
//                 },
//             },
//             {
//                 "refer 'jake' \'./path /to /jake.u\'",
//                 refer_statement{
//                     .m_tag = quoted_atom{"jake"},
//                     .m_file_path = quoted_atom{"./path /to /jake.u"},
//                 },
//             },
//             {
//                 "refer 1.1 \"./daniel.u\"",
//                 refer_statement{
//                     .m_tag = unquoted_atom{"1.1"},
//                     .m_file_path = quoted_atom{"./daniel.u"},
//                 },
//             },
//         };

//     for (const auto &[l_key, l_value] : l_test_cases)
//     {
//         std::stringstream l_ss(l_key);

//         refer_statement l_exp;

//         l_ss >> l_exp;

//         assert(l_exp == l_value);

//         // make sure the stringstream is not in failstate
//         assert(!l_ss.fail());

//         LOG("success, case: \"" << l_key << "\"" << std::endl);
//     }

//     std::vector<std::string> l_fail_cases =
//         {
//             "_",
//             "abc",
//             "",
//             "_ _",
//             "VariableTag Guide",
//             "VariableTag atom",
//             "VariableTag [elem0 elem1]",
//             "[] theorem",
//             "[X] theorem",
//             "[atom] theorem",
//             "axiom a0",
//             "axiom \'a0\'",
//             "axiom [tag] [expr]",
//             "axiom tag x",
//             "axiom a0 x"
//             "infer i0 x",
//             "infer [i0] x [theorem a0]",
//             "refer r0 x",
//             "guide g0 [",
//             "guide g0 [test] [redir]",
//             "guide g0 [V test] [redir]",
//             "guide g0 n [theorem a0]",
//             "guide g0 [] [theorem a0]",
//             "infer [",
//             "infer ]",
//             "infer \'i0",
//             "refer [jake] \'jake.u\'",
//             "refer jake ['not_a_quoted_atom']",
//         };

//     for (const auto &l_input : l_fail_cases)
//     {
//         std::stringstream l_ss(l_input);

//         refer_statement l_statement;

//         l_ss >> l_statement;

//         // ensure failure of extraction
//         assert(l_ss.fail());

//         LOG("success, case: expected failure extracting guide_statement: " << l_input << std::endl);
//     }
// }

// void test_parse_file_example_0()
// {
//     using unilog::atom;
//     using unilog::axiom_statement;
//     using unilog::guide_statement;
//     using unilog::infer_statement;
//     using unilog::lexeme;
//     using unilog::list_close;
//     using unilog::list_open;
//     using unilog::list_separator;
//     using unilog::quoted_atom;
//     using unilog::refer_statement;
//     using unilog::statement;
//     using unilog::term;
//     using unilog::unquoted_atom;
//     using unilog::variable;

//     std::ifstream l_if("./src/test_input_files/parser_example_0/main.ul");

//     std::stringstream l_file_contents;
//     if (!(l_file_contents << l_if.rdbuf())) // read in file content
//         throw std::runtime_error("Failed to read file");

//     std::list<statement> l_statements;
//     std::copy(std::istream_iterator<statement>(l_file_contents), std::istream_iterator<statement>(), std::back_inserter(l_statements));

//     assert(!l_file_contents.eof()); // invalid syntax, will be detected and cause failure
//     assert(l_statements == std::list<statement>(
//                                {
//                                    axiom_statement{
//                                        .m_tag = quoted_atom{"a0"},
//                                        .m_theorem = term{unquoted_atom{"test"}},
//                                    },
//                                }));
// }

// void test_parse_file_example_1()
// {
//     using unilog::atom;
//     using unilog::axiom_statement;
//     using unilog::guide_statement;
//     using unilog::infer_statement;
//     using unilog::lexeme;
//     using unilog::list_close;
//     using unilog::list_open;
//     using unilog::list_separator;
//     using unilog::quoted_atom;
//     using unilog::refer_statement;
//     using unilog::statement;
//     using unilog::term;
//     using unilog::unquoted_atom;
//     using unilog::variable;

//     std::ifstream l_if("./src/test_input_files/parser_example_1/main.ul");

//     std::stringstream l_file_contents;
//     if (!(l_file_contents << l_if.rdbuf())) // read in file content
//         throw std::runtime_error("Failed to read file");

//     std::list<statement> l_statements;
//     std::copy(std::istream_iterator<statement>(l_file_contents), std::istream_iterator<statement>(), std::back_inserter(l_statements));

//     assert(l_file_contents.eof()); // assert successful parse
//     assert(l_statements == std::list<statement>(
//                                {
//                                    axiom_statement{
//                                        .m_tag = unquoted_atom{"a0"},
//                                        .m_theorem = term{std::list<term>({
//                                            term{unquoted_atom{"if"}},
//                                            term{unquoted_atom{"y"}},
//                                            term{unquoted_atom{"x"}},
//                                        })},
//                                    },
//                                    axiom_statement{
//                                        .m_tag = unquoted_atom{"a1"},
//                                        .m_theorem = term{unquoted_atom{"x"}},
//                                    },

//                                }));
// }

void test_parser_main()
{
    constexpr bool ENABLE_DEBUG_LOGS = true;

    TEST(test_make_nil);
    TEST(test_make_atom);
    TEST(test_make_list);
    TEST(test_make_variable);
    TEST(test_parser_extract_prolog_expression);
    // TEST(test_parser_extract_axiom_statement);
    // TEST(test_parser_extract_guide_statement);
    // TEST(test_parser_extract_infer_statement);
    // TEST(test_parser_extract_refer_statement);
    // TEST(test_parse_file_example_0);
    // TEST(test_parse_file_example_1);
}

#endif
