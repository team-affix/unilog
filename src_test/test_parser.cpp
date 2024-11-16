#include <iostream>
#include <sstream>
#include <assert.h>
#include <map>
#include <vector>
#include <iterator>
#include <algorithm>
#include <random>
#include <fstream>

#include "test_utils.hpp"

#include "../src_lib/parser.hpp"
#include "../src_lib/variant_functions.hpp"

// Function signatures to test
std::istream &unilog::operator>>(std::istream &a_istream, unilog::term &a_prolog_expression);

////////////////////////////////
//// HELPER FUNCTIONS
////////////////////////////////

bool read_in_file(const std::string &a_file_path, std::stringstream &a_result)
{
    using unilog::statement;

    std::ifstream l_if(a_file_path);

    return (bool)(a_result << l_if.rdbuf()); // read in all content
}

////////////////////////////////
//// TESTS
////////////////////////////////

void test_cons()
{
    unilog::cons<int> l_cons{6};
}

void test_parser_extract_prolog_expression()
{
    constexpr bool ENABLE_DEBUG_LOGS = true;

    using unilog::atom;
    using unilog::lexeme;
    using unilog::list_close;
    using unilog::list_open;
    using unilog::quoted_atom;
    using unilog::term;
    using unilog::unquoted_atom;
    using unilog::variable;

    data_points<std::string, term>
        l_test_cases =
            {
                {
                    "if",
                    term{
                        unquoted_atom{
                            .m_text = "if",
                        },
                    },
                },
                {
                    "Var",
                    term{
                        variable{
                            .m_identifier = "Var",
                        },
                    },
                },
                {
                    "_",
                    term{
                        variable{
                            .m_identifier = "_",
                        },
                    },
                },
                {
                    // Test singular extraction not perturbed by later content
                    "_ abc",
                    term{
                        variable{
                            .m_identifier = "_",
                        },
                    },
                },
                {
                    "[]",
                    term{
                        std::list<term>{

                        },
                    },
                },
                {
                    "[] []",
                    term{
                        std::list<term>{

                        },
                    },
                },
                {
                    "[abc]",
                    term{
                        std::list<term>{
                            term{
                                unquoted_atom{
                                    "abc",
                                },
                            },
                        },
                    },
                },
                {
                    "[abc Var]",
                    term{
                        std::list<term>{
                            term{
                                unquoted_atom{
                                    "abc",
                                },
                            },
                            term{
                                variable{
                                    "Var",
                                },
                            },
                        },
                    },
                },
                {
                    "[abc Var def]",
                    term{
                        std::list<term>{
                            term{
                                unquoted_atom{
                                    "abc",
                                },
                            },
                            term{
                                variable{
                                    "Var",
                                },
                            },
                            term{
                                unquoted_atom{
                                    "def",
                                },
                            },
                        },
                    },
                },
                {
                    "[[]]",
                    term{
                        std::list<term>{
                            term{
                                std::list<term>{

                                },
                            },
                        },
                    },
                },
                {
                    "[abc []]",
                    term{
                        std::list<term>{
                            term{
                                unquoted_atom{
                                    "abc",
                                },
                            },
                            term{
                                std::list<term>{

                                },
                            },
                        },
                    },
                },
                {
                    "[abc [] def]",
                    term{
                        std::list<term>{
                            term{
                                unquoted_atom{
                                    "abc",
                                },
                            },
                            term{
                                std::list<term>{

                                },
                            },
                            term{
                                unquoted_atom{
                                    "def",
                                },
                            },
                        },
                    },
                },
                {
                    "[abc [123] def]",
                    term{
                        std::list<term>{
                            term{
                                unquoted_atom{
                                    "abc",
                                },
                            },
                            term{
                                std::list<term>{
                                    term{
                                        unquoted_atom{
                                            "123",
                                        },
                                    },
                                },
                            },
                            term{
                                unquoted_atom{
                                    "def",
                                },
                            },
                        },
                    },
                },
                {
                    "abc []",
                    term{
                        unquoted_atom{
                            "abc",
                        },
                    },
                },
                {
                    "[abc [123 [def [456 [ghi]]]]]",
                    term{
                        std::list<term>{
                            term{
                                unquoted_atom{
                                    "abc",
                                },
                            },
                            term{
                                std::list<term>{
                                    term{
                                        unquoted_atom{
                                            "123",
                                        },
                                    },
                                    term{
                                        std::list<term>{
                                            term{
                                                unquoted_atom{
                                                    "def",
                                                },
                                            },
                                            term{
                                                std::list<term>{
                                                    term{
                                                        unquoted_atom{
                                                            "456",
                                                        },
                                                    },
                                                    term{
                                                        std::list<term>{
                                                            term{
                                                                unquoted_atom{
                                                                    "ghi",
                                                                },
                                                            },
                                                        },
                                                    },
                                                },
                                            },
                                        },
                                    },
                                },
                            },
                        },
                    },
                },
                {
                    "[abc [_ _] [def A]]",
                    term{
                        std::list<term>{
                            term{
                                unquoted_atom{
                                    "abc",
                                },
                            },
                            term{
                                std::list<term>{
                                    term{
                                        variable{
                                            "_",
                                        },
                                    },
                                    term{
                                        variable{
                                            "_",
                                        },
                                    },
                                },
                            },
                            term{
                                std::list<term>{
                                    term{
                                        unquoted_atom{
                                            "def",
                                        },
                                    },
                                    term{
                                        variable{
                                            "A",
                                        },
                                    },
                                },
                            },
                        },
                    },
                },
                {
                    // Single atom, with special characters
                    "test!@.$^&*()",
                    term{
                        unquoted_atom{
                            "test!@.$^&*()",
                        },
                    },
                },
                {
                    // single atom, it will not fail even with the closing paren. this is because,
                    //     closing paren is a lexeme separator char and thus it will only extract the
                    //     first prolog expression, "abc"
                    "abc]",
                    term{
                        unquoted_atom{
                            "abc",
                        },
                    },
                },
                {
                    "[123 456 7.89]",
                    term{
                        std::list<term>{
                            term{
                                unquoted_atom{
                                    "123",
                                },
                            },
                            term{
                                unquoted_atom{
                                    "456",
                                },
                            },
                            term{
                                unquoted_atom{
                                    "7.89",
                                },
                            },
                        },
                    },
                },
                {
                    "[123 456 7.89 A [_ _]]",
                    term{
                        std::list<term>{
                            term{
                                unquoted_atom{
                                    "123",
                                },
                            },
                            term{
                                unquoted_atom{
                                    "456",
                                },
                            },
                            term{
                                unquoted_atom{
                                    "7.89",
                                },
                            },
                            term{
                                variable{
                                    "A",
                                },
                            },
                            term{
                                std::list<term>{
                                    term{
                                        variable{
                                            "_",
                                        },
                                    },
                                    term{
                                        variable{
                                            "_",
                                        },
                                    },
                                },
                            },
                        },
                    },
                },
                {
                    // Having fun, realistic scenario
                    "[if\n"
                    "    [add X Y Z]\n"
                    "    [and\n"
                    "        [cons X XH XT]\n"
                    "        [cons Y YH YT]\n"
                    "        [add [XH] [YH] [ZH]]\n"
                    "        [add XT YT ZT]\n"
                    "        [cons Z ZH ZT]\n"
                    "    ]\n"
                    "]\n",
                    term{
                        std::list<term>{
                            term{
                                unquoted_atom{
                                    "if",
                                },
                            },
                            term{
                                std::list<term>{
                                    term{
                                        unquoted_atom{
                                            "add",
                                        },
                                    },
                                    term{
                                        variable{
                                            "X",
                                        },
                                    },
                                    term{
                                        variable{
                                            "Y",
                                        },
                                    },
                                    term{
                                        variable{
                                            "Z",
                                        },
                                    },
                                },
                            },
                            term{
                                std::list<term>{
                                    term{
                                        unquoted_atom{
                                            "and",
                                        },
                                    },
                                    term{
                                        std::list<term>{
                                            term{
                                                unquoted_atom{
                                                    "cons",
                                                },
                                            },
                                            term{
                                                variable{
                                                    "X",
                                                },
                                            },
                                            term{
                                                variable{
                                                    "XH",
                                                },
                                            },
                                            term{
                                                variable{
                                                    "XT",
                                                },
                                            },
                                        },
                                    },
                                    term{
                                        std::list<term>{
                                            term{
                                                unquoted_atom{
                                                    "cons",
                                                },
                                            },
                                            term{
                                                variable{
                                                    "Y",
                                                },
                                            },
                                            term{
                                                variable{
                                                    "YH",
                                                },
                                            },
                                            term{
                                                variable{
                                                    "YT",
                                                },
                                            },
                                        },
                                    },
                                    term{
                                        std::list<term>{
                                            term{
                                                unquoted_atom{
                                                    "add",
                                                },
                                            },
                                            term{
                                                std::list<term>{
                                                    term{
                                                        variable{
                                                            "XH",
                                                        },
                                                    },
                                                },
                                            },
                                            term{
                                                std::list<term>{
                                                    term{
                                                        variable{
                                                            "YH",
                                                        },
                                                    },
                                                },
                                            },
                                            term{
                                                std::list<term>{
                                                    term{
                                                        variable{
                                                            "ZH",
                                                        },
                                                    },
                                                },
                                            },
                                        },
                                    },
                                    term{
                                        std::list<term>{
                                            term{
                                                unquoted_atom{
                                                    "add",
                                                },
                                            },
                                            term{
                                                variable{
                                                    "XT",
                                                },
                                            },
                                            term{
                                                variable{
                                                    "YT",
                                                },
                                            },
                                            term{
                                                variable{
                                                    "ZT",
                                                },
                                            },
                                        },
                                    },
                                    term{
                                        std::list<term>{
                                            term{
                                                unquoted_atom{
                                                    "cons",
                                                },
                                            },
                                            term{
                                                variable{
                                                    "Z",
                                                },
                                            },
                                            term{
                                                variable{
                                                    "ZH",
                                                },
                                            },
                                            term{
                                                variable{
                                                    "ZT",
                                                },
                                            },
                                        },
                                    },
                                },
                            },
                        },
                    },
                },
                {
                    "[[[] abc] 123]",
                    term{
                        std::list<term>{
                            term{
                                std::list<term>{
                                    term{
                                        std::list<term>{

                                        },
                                    },
                                    term{
                                        unquoted_atom{
                                            "abc",
                                        },
                                    },
                                },
                            },
                            term{
                                unquoted_atom{
                                    "123",
                                },
                            },
                        },
                    },
                },
            };

    for (const auto &[l_key, l_value] : l_test_cases)
    {
        std::stringstream l_ss(l_key);

        term l_exp;

        l_ss >> l_exp;

        assert(l_exp == l_value);

        assert(!l_ss.fail());

        LOG("success, case: \"" << l_key << "\"" << std::endl);
    }

    std::vector<std::string> l_expect_failure_inputs =
        {
            "[abc",
            "[[abc] [123]",
            "]",
            "\'",
            "\"",
            // "abc]", // this is NOT an expect failure input.
            // this is because it will only try to parse the first prolog expression before a lexeme separator char.

        };

    for (const auto &l_input : l_expect_failure_inputs)
    {
        std::stringstream l_ss(l_input);

        term l_exp;

        l_ss >> l_exp;

        // make sure the extraction was unsuccessful
        assert(l_ss.fail());

        LOG("success, expected throw, case: " << l_input << std::endl);
    }
}

void test_parser_extract_axiom_statement()
{
    constexpr bool ENABLE_DEBUG_LOGS = true;

    using unilog::atom;
    using unilog::axiom_statement;
    using unilog::lexeme;
    using unilog::list_close;
    using unilog::list_open;
    using unilog::quoted_atom;
    using unilog::term;
    using unilog::unquoted_atom;
    using unilog::variable;

    data_points<std::string, axiom_statement> l_test_cases =
        {
            {
                "axiom a0 x",
                axiom_statement{
                    .m_tag =
                        unquoted_atom{
                            "a0",
                        },
                    .m_theorem =
                        term{
                            unquoted_atom{
                                "x",
                            },
                        },
                },
            },
            {
                "axiom add_bc_0 [add [] L L]",
                axiom_statement{
                    .m_tag =
                        unquoted_atom{
                            "add_bc_0",
                        },
                    .m_theorem =
                        term{
                            std::list<term>{
                                term{
                                    unquoted_atom{
                                        "add",
                                    },
                                },
                                term{
                                    std::list<term>{

                                    },
                                },
                                term{
                                    variable{
                                        "L",
                                    },
                                },
                                term{
                                    variable{
                                        "L",
                                    },
                                },
                            },
                        },
                },
            },
            {
                "axiom @tag [awesome X]",
                axiom_statement{
                    .m_tag =
                        unquoted_atom{
                            "@tag",
                        },
                    .m_theorem =
                        term{
                            std::list<term>{
                                term{
                                    unquoted_atom{
                                        "awesome",
                                    },
                                },
                                term{
                                    variable{
                                        "X",
                                    },
                                },
                            },
                        },
                },
            },
            {
                "axiom tAg123@#$%^&*() _",
                axiom_statement{
                    .m_tag =
                        unquoted_atom{
                            "tAg123@#$%^&*()",
                        },
                    .m_theorem =
                        term{
                            variable{
                                "_",
                            },
                        },
                },
            },
            {
                "axiom tag[theorem]",
                axiom_statement{
                    .m_tag =
                        unquoted_atom{
                            "tag",
                        },
                    .m_theorem =
                        term{
                            std::list<term>{
                                term{
                                    unquoted_atom{
                                        "theorem",
                                    },
                                },
                            },
                        },
                },
            },
            {
                "axiom abc 123",
                axiom_statement{
                    .m_tag =
                        unquoted_atom{
                            "abc",
                        },
                    .m_theorem =
                        term{
                            unquoted_atom{
                                "123",
                            },
                        },
                },
            },
            {
                "axiom 123 [[[]] a 123]",
                axiom_statement{
                    .m_tag =
                        unquoted_atom{
                            "123",
                        },
                    .m_theorem =
                        term{
                            std::list<term>{
                                term{
                                    std::list<term>{

                                        term{
                                            std::list<term>{

                                            },
                                        },
                                    },
                                },
                                term{
                                    unquoted_atom{
                                        "a",
                                    },
                                },
                                term{
                                    unquoted_atom{
                                        "123",
                                    },
                                },
                            },
                        },
                },
            },
            {
                "axiom +/bc/0\n"
                "[+\n"
                "    []\n"
                "    L\n"
                "    L\n"
                "]",
                axiom_statement{
                    .m_tag =
                        unquoted_atom{
                            "+/bc/0",
                        },
                    .m_theorem =
                        term{
                            std::list<term>{
                                term{
                                    unquoted_atom{
                                        "+",
                                    },
                                },
                                term{
                                    std::list<term>{

                                    },
                                },
                                term{
                                    variable{
                                        "L",
                                    },
                                },
                                term{
                                    variable{
                                        "L",
                                    },
                                },
                            },
                        },
                },
            },
            {
                "axiom 123[\'! this is a \\t quotation\']",
                axiom_statement{
                    .m_tag =
                        unquoted_atom{
                            "123",
                        },
                    .m_theorem =
                        term{
                            std::list<term>{
                                term{
                                    quoted_atom{
                                        "! this is a \t quotation",
                                    },
                                },
                            },
                        },
                },
            },
            {
                "axiom \'tag\' theorem",
                axiom_statement{
                    .m_tag =
                        quoted_atom{
                            "tag",
                        },
                    .m_theorem =
                        term{
                            unquoted_atom{
                                "theorem",
                            },
                        },
                },
            },
            {
                "axiom \"tag\" theorem",
                axiom_statement{
                    .m_tag =
                        quoted_atom{
                            "tag",
                        },
                    .m_theorem =
                        term{
                            unquoted_atom{
                                "theorem",
                            },
                        },
                },
            },
        };

    for (const auto &[l_key, l_value] : l_test_cases)
    {
        std::stringstream l_ss(l_key);

        axiom_statement l_exp;

        l_ss >> l_exp;

        assert(l_exp == l_value);

        // make sure the stringstream is not in failstate
        assert(!l_ss.fail());

        LOG("success, case: \"" << l_key << "\"" << std::endl);
    }

    std::vector<std::string> l_fail_cases =
        {
            "",
            "abc",
            "_ _",
            "VariableTag Theorem",
            "VariableTag atom",
            "VariableTag [elem0 elem1]",
            "[] theorem",
            "[X] theorem",
            "[atom] theorem",
            "axiom a0",
            "axiom \'a0\'",
            "axiom [tag] [expr]",
            "guide a0 x",
            "infer i0 x",
            "refer r0 x",
        };

    for (const auto &l_input : l_fail_cases)
    {
        std::stringstream l_ss(l_input);

        axiom_statement l_axiom_statement;

        l_ss >> l_axiom_statement;

        // ensure failure of extraction
        assert(l_ss.fail());

        LOG("success, case: expected failure extracting axiom_statement: " << l_input << std::endl);
    }
}

void test_parser_extract_guide_statement()
{
    constexpr bool ENABLE_DEBUG_LOGS = true;

    using unilog::atom;
    using unilog::guide_statement;
    using unilog::lexeme;
    using unilog::list_close;
    using unilog::list_open;
    using unilog::quoted_atom;
    using unilog::term;
    using unilog::unquoted_atom;
    using unilog::variable;

    data_points<std::string, guide_statement> l_test_cases =
        {
            {
                "guide g_add_bc []\n"
                "[gor\n"
                "    add_bc_0\n"
                "    add_bc_1\n"
                "    add_bc_2\n"
                "    add_bc_3\n"
                "    add_bc_4\n"
                "    add_bc_5\n"
                "]\n",
                guide_statement{
                    .m_tag =
                        term{
                            unquoted_atom{
                                "g_add_bc",
                            },
                        },
                    .m_args = std::list<variable>({}),
                    .m_guide =
                        term{
                            std::list<term>{
                                term{
                                    unquoted_atom{
                                        "gor",
                                    },
                                },
                                term{
                                    unquoted_atom{
                                        "add_bc_0",
                                    },
                                },
                                term{
                                    unquoted_atom{
                                        "add_bc_1",
                                    },
                                },
                                term{
                                    unquoted_atom{
                                        "add_bc_2",
                                    },
                                },
                                term{
                                    unquoted_atom{
                                        "add_bc_3",
                                    },
                                },
                                term{
                                    unquoted_atom{
                                        "add_bc_4",
                                    },
                                },
                                term{
                                    unquoted_atom{
                                        "add_bc_5",
                                    },
                                },
                            },
                        },
                },
            },
            {
                "guide g0[][bind K [theorem a0]]",
                guide_statement{
                    .m_tag = unquoted_atom{"g0"},
                    .m_args = std::list<variable>({}),
                    .m_guide = term{std::list<term>({
                        term{
                            unquoted_atom{"bind"},
                        },
                        term{
                            variable{"K"},
                        },
                        term{std::list<term>({
                            term{
                                unquoted_atom{"theorem"},
                            },
                            term{
                                unquoted_atom{"a0"},
                            },
                        })},
                    })}},
            },
            {"guide \"g\"[] [sub thm [theorem a0] [theorem a1]]",
             guide_statement{
                 .m_tag = quoted_atom{"g"},
                 .m_args = std::list<variable>({}),
                 .m_guide = term{
                     std::list<term>({
                         term{unquoted_atom{"sub"}},
                         term{unquoted_atom{"thm"}},
                         term{std::list<term>({
                             term{unquoted_atom{"theorem"}},
                             term{unquoted_atom{"a0"}},
                         })},
                         term{std::list<term>({
                             term{unquoted_atom{"theorem"}},
                             term{unquoted_atom{"a1"}},
                         })},
                     })}}},
            {
                "guide \"g\" [Subgoal Subguide][sub Subgoal Subguide [theorem a1]]",
                guide_statement{
                    .m_tag = quoted_atom{"g"},
                    .m_args = std::list<variable>({
                        variable{"Subgoal"},
                        variable{"Subguide"},
                    }),
                    .m_guide = term{std::list<term>({
                        term{
                            unquoted_atom{"sub"},
                        },
                        term{
                            variable{"Subgoal"},
                        },
                        term{
                            variable{"Subguide"},
                        },
                        term{std::list<term>({
                            term{
                                unquoted_atom{"theorem"},
                            },
                            term{
                                unquoted_atom{"a1"},
                            },
                        })},
                    })},
                },
            },
            {
                "guide gt [] [mp [theorem a0] [theorem a1]]",
                guide_statement{
                    .m_tag = unquoted_atom{"gt"},
                    .m_args = std::list<variable>({}),
                    .m_guide = term{
                        std::list<term>({
                            term{
                                unquoted_atom{"mp"},
                            },
                            term{
                                std::list<term>({
                                    term{
                                        unquoted_atom{"theorem"},
                                    },
                                    term{
                                        unquoted_atom{"a0"},
                                    },
                                }),
                            },
                            term{
                                std::list<term>({
                                    term{
                                        unquoted_atom{"theorem"},
                                    },
                                    term{
                                        unquoted_atom{"a1"},
                                    },
                                }),
                            },
                        }),
                    },
                },
            },
        };

    for (const auto &[l_key, l_value] : l_test_cases)
    {
        std::stringstream l_ss(l_key);

        guide_statement l_exp;

        l_ss >> l_exp;

        assert(l_exp == l_value);

        // make sure the stringstream is not in failstate
        assert(!l_ss.fail());

        LOG("success, case: \"" << l_key << "\"" << std::endl);
    }

    std::vector<std::string> l_fail_cases =
        {
            "_",
            "abc",
            "",
            "_ _",
            "VariableTag Guide",
            "VariableTag atom",
            "VariableTag [elem0 elem1]",
            "[] theorem",
            "[X] theorem",
            "[atom] theorem",
            "axiom a0",
            "axiom \'a0\'",
            "axiom [tag] [expr]",
            "axiom a0 x"
            "infer i0 x",
            "refer r0 x",
            "guide g0 [",
            "guide g0 [test] [redir]",   // arg list is not comprised of variables
            "guide g0 [V test] [redir]", // arg list is not comprised of only variables
            "guide g0 n [theorem a0]",
        };

    for (const auto &l_input : l_fail_cases)
    {
        std::stringstream l_ss(l_input);

        guide_statement l_statement;

        l_ss >> l_statement;

        // ensure failure of extraction
        assert(l_ss.fail());

        LOG("success, case: expected failure extracting guide_statement: " << l_input << std::endl);
    }
}

void test_parser_extract_infer_statement()
{
    constexpr bool ENABLE_DEBUG_LOGS = true;

    using unilog::atom;
    using unilog::infer_statement;
    using unilog::lexeme;
    using unilog::list_close;
    using unilog::list_open;
    using unilog::quoted_atom;
    using unilog::term;
    using unilog::unquoted_atom;
    using unilog::variable;

    data_points<std::string, infer_statement> l_test_cases =
        {
            {
                "infer i0 [claim daniel y] [bout daniel [mp [theorem a0] [theorem a1]]]",
                infer_statement{
                    .m_tag = unquoted_atom{"i0"},
                    .m_theorem = term{std::list<term>({
                        term{unquoted_atom{"claim"}},
                        term{unquoted_atom{"daniel"}},
                        term{unquoted_atom{"y"}},
                    })},
                    .m_guide = term{std::list<term>({
                        term{unquoted_atom{"bout"}},
                        term{unquoted_atom{"daniel"}},
                        term{std::list<term>({
                            term{unquoted_atom{"mp"}},
                            term{std::list<term>({
                                term{unquoted_atom{"theorem"}},
                                term{unquoted_atom{"a0"}},
                            })},
                            term{std::list<term>({
                                term{unquoted_atom{"theorem"}},
                                term{unquoted_atom{"a1"}},
                            })},
                        })},
                    })},
                },
            },
            {
                "infer 'i0' [claim daniel y] [bout daniel [mp [theorem a0] [theorem a1]]]",
                infer_statement{
                    .m_tag = quoted_atom{"i0"},
                    .m_theorem = term{std::list<term>({
                        term{unquoted_atom{"claim"}},
                        term{unquoted_atom{"daniel"}},
                        term{unquoted_atom{"y"}},
                    })},
                    .m_guide = term{std::list<term>({
                        term{unquoted_atom{"bout"}},
                        term{unquoted_atom{"daniel"}},
                        term{std::list<term>({
                            term{unquoted_atom{"mp"}},
                            term{std::list<term>({
                                term{unquoted_atom{"theorem"}},
                                term{unquoted_atom{"a0"}},
                            })},
                            term{std::list<term>({
                                term{unquoted_atom{"theorem"}},
                                term{unquoted_atom{"a1"}},
                            })},
                        })},
                    })},
                },
            },
            {
                "infer 'i0' [claim daniel y] [bout daniel [dout daniel [mp [theorem a2] [theorem a3]]]]",
                infer_statement{
                    .m_tag = quoted_atom{"i0"},
                    .m_theorem = term{std::list<term>({
                        term{unquoted_atom{"claim"}},
                        term{unquoted_atom{"daniel"}},
                        term{unquoted_atom{"y"}},
                    })},
                    .m_guide = term{std::list<term>({
                        term{unquoted_atom{"bout"}},
                        term{unquoted_atom{"daniel"}},
                        term{std::list<term>({
                            term{unquoted_atom{"dout"}},
                            term{unquoted_atom{"daniel"}},
                            term{std::list<term>({
                                term{unquoted_atom{"mp"}},
                                term{std::list<term>({
                                    term{unquoted_atom{"theorem"}},
                                    term{unquoted_atom{"a2"}},
                                })},
                                term{std::list<term>({
                                    term{unquoted_atom{"theorem"}},
                                    term{unquoted_atom{"a3"}},
                                })},
                            })},
                        })},
                    })},
                },
            },
            {
                "infer 'i0' [claim daniel y] a0",
                infer_statement{
                    .m_tag = quoted_atom{"i0"},
                    .m_theorem = term{std::list<term>({
                        term{unquoted_atom{"claim"}},
                        term{unquoted_atom{"daniel"}},
                        term{unquoted_atom{"y"}},
                    })},
                    .m_guide = term{unquoted_atom{"a0"}},
                },
            },
        };

    for (const auto &[l_key, l_value] : l_test_cases)
    {
        std::stringstream l_ss(l_key);

        infer_statement l_exp;

        l_ss >> l_exp;

        assert(l_exp == l_value);

        // make sure the stringstream is not in failstate
        assert(!l_ss.fail());

        LOG("success, case: \"" << l_key << "\"" << std::endl);
    }

    std::vector<std::string> l_fail_cases =
        {
            "_",
            "abc",
            "",
            "_ _",
            "VariableTag Guide",
            "VariableTag atom",
            "VariableTag [elem0 elem1]",
            "[] theorem",
            "[X] theorem",
            "[atom] theorem",
            "axiom a0",
            "axiom \'a0\'",
            "axiom [tag] [expr]",
            "axiom tag x",
            "axiom a0 x"
            "infer i0 x",
            "infer [i0] x [theorem a0]", // tag is not atomic
            "refer r0 x",
            "guide g0 [",
            "guide g0 [test] [redir]",
            "guide g0 [V test] [redir]",
            "guide g0 n [theorem a0]",
            "guide g0 [] [theorem a0]",
            "infer [",
            "infer ]",
            "infer \'i0",
        };

    for (const auto &l_input : l_fail_cases)
    {
        std::stringstream l_ss(l_input);

        infer_statement l_statement;

        l_ss >> l_statement;

        // ensure failure of extraction
        assert(l_ss.fail());

        LOG("success, case: expected failure extracting guide_statement: " << l_input << std::endl);
    }
}

void test_parser_extract_refer_statement()
{
    constexpr bool ENABLE_DEBUG_LOGS = true;

    using unilog::atom;
    using unilog::lexeme;
    using unilog::list_close;
    using unilog::list_open;
    using unilog::quoted_atom;
    using unilog::refer_statement;
    using unilog::term;
    using unilog::unquoted_atom;
    using unilog::variable;

    data_points<std::string, refer_statement> l_test_cases =
        {
            {
                "refer leon \'./path/to/leon.u\'",
                refer_statement{
                    .m_tag = unquoted_atom{"leon"},
                    .m_file_path = quoted_atom{"./path/to/leon.u"},
                },
            },
            {
                "refer 'jake' \'./path /to /jake.u\'",
                refer_statement{
                    .m_tag = quoted_atom{"jake"},
                    .m_file_path = quoted_atom{"./path /to /jake.u"},
                },
            },
            {
                "refer 1.1 \"./daniel.u\"",
                refer_statement{
                    .m_tag = unquoted_atom{"1.1"},
                    .m_file_path = quoted_atom{"./daniel.u"},
                },
            },
        };

    for (const auto &[l_key, l_value] : l_test_cases)
    {
        std::stringstream l_ss(l_key);

        refer_statement l_exp;

        l_ss >> l_exp;

        assert(l_exp == l_value);

        // make sure the stringstream is not in failstate
        assert(!l_ss.fail());

        LOG("success, case: \"" << l_key << "\"" << std::endl);
    }

    std::vector<std::string> l_fail_cases =
        {
            "_",
            "abc",
            "",
            "_ _",
            "VariableTag Guide",
            "VariableTag atom",
            "VariableTag [elem0 elem1]",
            "[] theorem",
            "[X] theorem",
            "[atom] theorem",
            "axiom a0",
            "axiom \'a0\'",
            "axiom [tag] [expr]",
            "axiom tag x",
            "axiom a0 x"
            "infer i0 x",
            "infer [i0] x [theorem a0]",
            "refer r0 x",
            "guide g0 [",
            "guide g0 [test] [redir]",
            "guide g0 [V test] [redir]",
            "guide g0 n [theorem a0]",
            "guide g0 [] [theorem a0]",
            "infer [",
            "infer ]",
            "infer \'i0",
            "refer [jake] \'jake.u\'",
            "refer jake ['not_a_quoted_atom']",
        };

    for (const auto &l_input : l_fail_cases)
    {
        std::stringstream l_ss(l_input);

        refer_statement l_statement;

        l_ss >> l_statement;

        // ensure failure of extraction
        assert(l_ss.fail());

        LOG("success, case: expected failure extracting guide_statement: " << l_input << std::endl);
    }
}

void test_parse_file_example_0()
{
    using unilog::atom;
    using unilog::axiom_statement;
    using unilog::guide_statement;
    using unilog::infer_statement;
    using unilog::lexeme;
    using unilog::list_close;
    using unilog::list_open;
    using unilog::list_separator;
    using unilog::quoted_atom;
    using unilog::refer_statement;
    using unilog::statement;
    using unilog::term;
    using unilog::unquoted_atom;
    using unilog::variable;

    std::ifstream l_if("./src_test/example_unilog_files/parser_example_0/main.ul");

    std::stringstream l_file_contents;
    if (!(l_file_contents << l_if.rdbuf())) // read in file content
        throw std::runtime_error("Failed to read file");

    std::list<statement> l_statements;
    std::copy(std::istream_iterator<statement>(l_file_contents), std::istream_iterator<statement>(), std::back_inserter(l_statements));

    assert(!l_file_contents.eof()); // invalid syntax, will be detected and cause failure
    assert(l_statements == std::list<statement>(
                               {
                                   axiom_statement{
                                       .m_tag = quoted_atom{"a0"},
                                       .m_theorem = term{unquoted_atom{"test"}},
                                   },
                               }));
}

void test_parse_file_example_1()
{
    using unilog::atom;
    using unilog::axiom_statement;
    using unilog::guide_statement;
    using unilog::infer_statement;
    using unilog::lexeme;
    using unilog::list_close;
    using unilog::list_open;
    using unilog::list_separator;
    using unilog::quoted_atom;
    using unilog::refer_statement;
    using unilog::statement;
    using unilog::term;
    using unilog::unquoted_atom;
    using unilog::variable;

    std::ifstream l_if("./src_test/example_unilog_files/parser_example_1/main.ul");

    std::stringstream l_file_contents;
    if (!(l_file_contents << l_if.rdbuf())) // read in file content
        throw std::runtime_error("Failed to read file");

    std::list<statement> l_statements;
    std::copy(std::istream_iterator<statement>(l_file_contents), std::istream_iterator<statement>(), std::back_inserter(l_statements));

    assert(l_file_contents.eof()); // assert successful parse
    assert(l_statements == std::list<statement>(
                               {
                                   axiom_statement{
                                       .m_tag = unquoted_atom{"a0"},
                                       .m_theorem = term{std::list<term>({
                                           term{unquoted_atom{"if"}},
                                           term{unquoted_atom{"y"}},
                                           term{unquoted_atom{"x"}},
                                       })},
                                   },
                                   axiom_statement{
                                       .m_tag = unquoted_atom{"a1"},
                                       .m_theorem = term{unquoted_atom{"x"}},
                                   },

                               }));
}

void test_parser_main()
{
    constexpr bool ENABLE_DEBUG_LOGS = true;

    TEST(test_cons);
    TEST(test_parser_extract_prolog_expression);
    TEST(test_parser_extract_axiom_statement);
    TEST(test_parser_extract_guide_statement);
    TEST(test_parser_extract_infer_statement);
    TEST(test_parser_extract_refer_statement);
    TEST(test_parse_file_example_0);
    TEST(test_parse_file_example_1);
}
