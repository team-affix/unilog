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
std::istream &unilog::operator>>(std::istream &a_istream, unilog::prolog_expression &a_prolog_expression);

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

void test_parser_extract_prolog_expression()
{
    constexpr bool ENABLE_DEBUG_LOGS = true;

    using unilog::atom;
    using unilog::lexeme;
    using unilog::list_close;
    using unilog::list_open;
    using unilog::prolog_expression;
    using unilog::quoted_atom;
    using unilog::unquoted_atom;
    using unilog::variable;

    data_points<std::string, prolog_expression>
        l_test_cases =
            {
                {
                    "if",
                    prolog_expression{
                        unquoted_atom{
                            .m_text = "if",
                        },
                    },
                },
                {
                    "Var",
                    prolog_expression{
                        variable{
                            .m_identifier = "Var",
                        },
                    },
                },
                {
                    "_",
                    prolog_expression{
                        variable{
                            .m_identifier = "_",
                        },
                    },
                },
                {
                    // Test singular extraction not perturbed by later content
                    "_ abc",
                    prolog_expression{
                        variable{
                            .m_identifier = "_",
                        },
                    },
                },
                {
                    "[]",
                    prolog_expression{
                        std::list<prolog_expression>{

                        },
                    },
                },
                {
                    "[] []",
                    prolog_expression{
                        std::list<prolog_expression>{

                        },
                    },
                },
                {
                    "[abc]",
                    prolog_expression{
                        std::list<prolog_expression>{
                            prolog_expression{
                                unquoted_atom{
                                    "abc",
                                },
                            },
                        },
                    },
                },
                {
                    "[abc Var]",
                    prolog_expression{
                        std::list<prolog_expression>{
                            prolog_expression{
                                unquoted_atom{
                                    "abc",
                                },
                            },
                            prolog_expression{
                                variable{
                                    "Var",
                                },
                            },
                        },
                    },
                },
                {
                    "[abc Var def]",
                    prolog_expression{
                        std::list<prolog_expression>{
                            prolog_expression{
                                unquoted_atom{
                                    "abc",
                                },
                            },
                            prolog_expression{
                                variable{
                                    "Var",
                                },
                            },
                            prolog_expression{
                                unquoted_atom{
                                    "def",
                                },
                            },
                        },
                    },
                },
                {
                    "[[]]",
                    prolog_expression{
                        std::list<prolog_expression>{
                            prolog_expression{
                                std::list<prolog_expression>{

                                },
                            },
                        },
                    },
                },
                {
                    "[abc []]",
                    prolog_expression{
                        std::list<prolog_expression>{
                            prolog_expression{
                                unquoted_atom{
                                    "abc",
                                },
                            },
                            prolog_expression{
                                std::list<prolog_expression>{

                                },
                            },
                        },
                    },
                },
                {
                    "[abc [] def]",
                    prolog_expression{
                        std::list<prolog_expression>{
                            prolog_expression{
                                unquoted_atom{
                                    "abc",
                                },
                            },
                            prolog_expression{
                                std::list<prolog_expression>{

                                },
                            },
                            prolog_expression{
                                unquoted_atom{
                                    "def",
                                },
                            },
                        },
                    },
                },
                {
                    "[abc [123] def]",
                    prolog_expression{
                        std::list<prolog_expression>{
                            prolog_expression{
                                unquoted_atom{
                                    "abc",
                                },
                            },
                            prolog_expression{
                                std::list<prolog_expression>{
                                    prolog_expression{
                                        unquoted_atom{
                                            "123",
                                        },
                                    },
                                },
                            },
                            prolog_expression{
                                unquoted_atom{
                                    "def",
                                },
                            },
                        },
                    },
                },
                {
                    "abc []",
                    prolog_expression{
                        unquoted_atom{
                            "abc",
                        },
                    },
                },
                {
                    "[abc [123 [def [456 [ghi]]]]]",
                    prolog_expression{
                        std::list<prolog_expression>{
                            prolog_expression{
                                unquoted_atom{
                                    "abc",
                                },
                            },
                            prolog_expression{
                                std::list<prolog_expression>{
                                    prolog_expression{
                                        unquoted_atom{
                                            "123",
                                        },
                                    },
                                    prolog_expression{
                                        std::list<prolog_expression>{
                                            prolog_expression{
                                                unquoted_atom{
                                                    "def",
                                                },
                                            },
                                            prolog_expression{
                                                std::list<prolog_expression>{
                                                    prolog_expression{
                                                        unquoted_atom{
                                                            "456",
                                                        },
                                                    },
                                                    prolog_expression{
                                                        std::list<prolog_expression>{
                                                            prolog_expression{
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
                    prolog_expression{
                        std::list<prolog_expression>{
                            prolog_expression{
                                unquoted_atom{
                                    "abc",
                                },
                            },
                            prolog_expression{
                                std::list<prolog_expression>{
                                    prolog_expression{
                                        variable{
                                            "_",
                                        },
                                    },
                                    prolog_expression{
                                        variable{
                                            "_",
                                        },
                                    },
                                },
                            },
                            prolog_expression{
                                std::list<prolog_expression>{
                                    prolog_expression{
                                        unquoted_atom{
                                            "def",
                                        },
                                    },
                                    prolog_expression{
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
                    prolog_expression{
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
                    prolog_expression{
                        unquoted_atom{
                            "abc",
                        },
                    },
                },
                {
                    "[123 456 7.89]",
                    prolog_expression{
                        std::list<prolog_expression>{
                            prolog_expression{
                                unquoted_atom{
                                    "123",
                                },
                            },
                            prolog_expression{
                                unquoted_atom{
                                    "456",
                                },
                            },
                            prolog_expression{
                                unquoted_atom{
                                    "7.89",
                                },
                            },
                        },
                    },
                },
                {
                    "[123 456 7.89 A [_ _]]",
                    prolog_expression{
                        std::list<prolog_expression>{
                            prolog_expression{
                                unquoted_atom{
                                    "123",
                                },
                            },
                            prolog_expression{
                                unquoted_atom{
                                    "456",
                                },
                            },
                            prolog_expression{
                                unquoted_atom{
                                    "7.89",
                                },
                            },
                            prolog_expression{
                                variable{
                                    "A",
                                },
                            },
                            prolog_expression{
                                std::list<prolog_expression>{
                                    prolog_expression{
                                        variable{
                                            "_",
                                        },
                                    },
                                    prolog_expression{
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
                    prolog_expression{
                        std::list<prolog_expression>{
                            prolog_expression{
                                unquoted_atom{
                                    "if",
                                },
                            },
                            prolog_expression{
                                std::list<prolog_expression>{
                                    prolog_expression{
                                        unquoted_atom{
                                            "add",
                                        },
                                    },
                                    prolog_expression{
                                        variable{
                                            "X",
                                        },
                                    },
                                    prolog_expression{
                                        variable{
                                            "Y",
                                        },
                                    },
                                    prolog_expression{
                                        variable{
                                            "Z",
                                        },
                                    },
                                },
                            },
                            prolog_expression{
                                std::list<prolog_expression>{
                                    prolog_expression{
                                        unquoted_atom{
                                            "and",
                                        },
                                    },
                                    prolog_expression{
                                        std::list<prolog_expression>{
                                            prolog_expression{
                                                unquoted_atom{
                                                    "cons",
                                                },
                                            },
                                            prolog_expression{
                                                variable{
                                                    "X",
                                                },
                                            },
                                            prolog_expression{
                                                variable{
                                                    "XH",
                                                },
                                            },
                                            prolog_expression{
                                                variable{
                                                    "XT",
                                                },
                                            },
                                        },
                                    },
                                    prolog_expression{
                                        std::list<prolog_expression>{
                                            prolog_expression{
                                                unquoted_atom{
                                                    "cons",
                                                },
                                            },
                                            prolog_expression{
                                                variable{
                                                    "Y",
                                                },
                                            },
                                            prolog_expression{
                                                variable{
                                                    "YH",
                                                },
                                            },
                                            prolog_expression{
                                                variable{
                                                    "YT",
                                                },
                                            },
                                        },
                                    },
                                    prolog_expression{
                                        std::list<prolog_expression>{
                                            prolog_expression{
                                                unquoted_atom{
                                                    "add",
                                                },
                                            },
                                            prolog_expression{
                                                std::list<prolog_expression>{
                                                    prolog_expression{
                                                        variable{
                                                            "XH",
                                                        },
                                                    },
                                                },
                                            },
                                            prolog_expression{
                                                std::list<prolog_expression>{
                                                    prolog_expression{
                                                        variable{
                                                            "YH",
                                                        },
                                                    },
                                                },
                                            },
                                            prolog_expression{
                                                std::list<prolog_expression>{
                                                    prolog_expression{
                                                        variable{
                                                            "ZH",
                                                        },
                                                    },
                                                },
                                            },
                                        },
                                    },
                                    prolog_expression{
                                        std::list<prolog_expression>{
                                            prolog_expression{
                                                unquoted_atom{
                                                    "add",
                                                },
                                            },
                                            prolog_expression{
                                                variable{
                                                    "XT",
                                                },
                                            },
                                            prolog_expression{
                                                variable{
                                                    "YT",
                                                },
                                            },
                                            prolog_expression{
                                                variable{
                                                    "ZT",
                                                },
                                            },
                                        },
                                    },
                                    prolog_expression{
                                        std::list<prolog_expression>{
                                            prolog_expression{
                                                unquoted_atom{
                                                    "cons",
                                                },
                                            },
                                            prolog_expression{
                                                variable{
                                                    "Z",
                                                },
                                            },
                                            prolog_expression{
                                                variable{
                                                    "ZH",
                                                },
                                            },
                                            prolog_expression{
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
                    prolog_expression{
                        std::list<prolog_expression>{
                            prolog_expression{
                                std::list<prolog_expression>{
                                    prolog_expression{
                                        std::list<prolog_expression>{

                                        },
                                    },
                                    prolog_expression{
                                        unquoted_atom{
                                            "abc",
                                        },
                                    },
                                },
                            },
                            prolog_expression{
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

        prolog_expression l_exp;

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

        prolog_expression l_exp;

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
    using unilog::prolog_expression;
    using unilog::quoted_atom;
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
                        prolog_expression{
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
                        prolog_expression{
                            std::list<prolog_expression>{
                                prolog_expression{
                                    unquoted_atom{
                                        "add",
                                    },
                                },
                                prolog_expression{
                                    std::list<prolog_expression>{

                                    },
                                },
                                prolog_expression{
                                    variable{
                                        "L",
                                    },
                                },
                                prolog_expression{
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
                        prolog_expression{
                            std::list<prolog_expression>{
                                prolog_expression{
                                    unquoted_atom{
                                        "awesome",
                                    },
                                },
                                prolog_expression{
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
                        prolog_expression{
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
                        prolog_expression{
                            std::list<prolog_expression>{
                                prolog_expression{
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
                        prolog_expression{
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
                        prolog_expression{
                            std::list<prolog_expression>{
                                prolog_expression{
                                    std::list<prolog_expression>{

                                        prolog_expression{
                                            std::list<prolog_expression>{

                                            },
                                        },
                                    },
                                },
                                prolog_expression{
                                    unquoted_atom{
                                        "a",
                                    },
                                },
                                prolog_expression{
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
                        prolog_expression{
                            std::list<prolog_expression>{
                                prolog_expression{
                                    unquoted_atom{
                                        "+",
                                    },
                                },
                                prolog_expression{
                                    std::list<prolog_expression>{

                                    },
                                },
                                prolog_expression{
                                    variable{
                                        "L",
                                    },
                                },
                                prolog_expression{
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
                        prolog_expression{
                            std::list<prolog_expression>{
                                prolog_expression{
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
                        prolog_expression{
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
                        prolog_expression{
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
    using unilog::prolog_expression;
    using unilog::quoted_atom;
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
                        prolog_expression{
                            unquoted_atom{
                                "g_add_bc",
                            },
                        },
                    .m_args = std::list<variable>({}),
                    .m_guide =
                        prolog_expression{
                            std::list<prolog_expression>{
                                prolog_expression{
                                    unquoted_atom{
                                        "gor",
                                    },
                                },
                                prolog_expression{
                                    unquoted_atom{
                                        "add_bc_0",
                                    },
                                },
                                prolog_expression{
                                    unquoted_atom{
                                        "add_bc_1",
                                    },
                                },
                                prolog_expression{
                                    unquoted_atom{
                                        "add_bc_2",
                                    },
                                },
                                prolog_expression{
                                    unquoted_atom{
                                        "add_bc_3",
                                    },
                                },
                                prolog_expression{
                                    unquoted_atom{
                                        "add_bc_4",
                                    },
                                },
                                prolog_expression{
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
                    .m_guide = prolog_expression{std::list<prolog_expression>({
                        prolog_expression{
                            unquoted_atom{"bind"},
                        },
                        prolog_expression{
                            variable{"K"},
                        },
                        prolog_expression{std::list<prolog_expression>({
                            prolog_expression{
                                unquoted_atom{"theorem"},
                            },
                            prolog_expression{
                                unquoted_atom{"a0"},
                            },
                        })},
                    })}},
            },
            {"guide \"g\"[] [sub thm [theorem a0] [theorem a1]]",
             guide_statement{
                 .m_tag = quoted_atom{"g"},
                 .m_args = std::list<variable>({}),
                 .m_guide = prolog_expression{
                     std::list<prolog_expression>({
                         prolog_expression{unquoted_atom{"sub"}},
                         prolog_expression{unquoted_atom{"thm"}},
                         prolog_expression{std::list<prolog_expression>({
                             prolog_expression{unquoted_atom{"theorem"}},
                             prolog_expression{unquoted_atom{"a0"}},
                         })},
                         prolog_expression{std::list<prolog_expression>({
                             prolog_expression{unquoted_atom{"theorem"}},
                             prolog_expression{unquoted_atom{"a1"}},
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
                    .m_guide = prolog_expression{std::list<prolog_expression>({
                        prolog_expression{
                            unquoted_atom{"sub"},
                        },
                        prolog_expression{
                            variable{"Subgoal"},
                        },
                        prolog_expression{
                            variable{"Subguide"},
                        },
                        prolog_expression{std::list<prolog_expression>({
                            prolog_expression{
                                unquoted_atom{"theorem"},
                            },
                            prolog_expression{
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
                    .m_guide = prolog_expression{
                        std::list<prolog_expression>({
                            prolog_expression{
                                unquoted_atom{"mp"},
                            },
                            prolog_expression{
                                std::list<prolog_expression>({
                                    prolog_expression{
                                        unquoted_atom{"theorem"},
                                    },
                                    prolog_expression{
                                        unquoted_atom{"a0"},
                                    },
                                }),
                            },
                            prolog_expression{
                                std::list<prolog_expression>({
                                    prolog_expression{
                                        unquoted_atom{"theorem"},
                                    },
                                    prolog_expression{
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
    using unilog::prolog_expression;
    using unilog::quoted_atom;
    using unilog::unquoted_atom;
    using unilog::variable;

    data_points<std::string, infer_statement> l_test_cases =
        {
            {
                "infer i0 [claim daniel y] [bout daniel [mp [theorem a0] [theorem a1]]]",
                infer_statement{
                    .m_tag = unquoted_atom{"i0"},
                    .m_theorem = prolog_expression{std::list<prolog_expression>({
                        prolog_expression{unquoted_atom{"claim"}},
                        prolog_expression{unquoted_atom{"daniel"}},
                        prolog_expression{unquoted_atom{"y"}},
                    })},
                    .m_guide = prolog_expression{std::list<prolog_expression>({
                        prolog_expression{unquoted_atom{"bout"}},
                        prolog_expression{unquoted_atom{"daniel"}},
                        prolog_expression{std::list<prolog_expression>({
                            prolog_expression{unquoted_atom{"mp"}},
                            prolog_expression{std::list<prolog_expression>({
                                prolog_expression{unquoted_atom{"theorem"}},
                                prolog_expression{unquoted_atom{"a0"}},
                            })},
                            prolog_expression{std::list<prolog_expression>({
                                prolog_expression{unquoted_atom{"theorem"}},
                                prolog_expression{unquoted_atom{"a1"}},
                            })},
                        })},
                    })},
                },
            },
            {
                "infer 'i0' [claim daniel y] [bout daniel [mp [theorem a0] [theorem a1]]]",
                infer_statement{
                    .m_tag = quoted_atom{"i0"},
                    .m_theorem = prolog_expression{std::list<prolog_expression>({
                        prolog_expression{unquoted_atom{"claim"}},
                        prolog_expression{unquoted_atom{"daniel"}},
                        prolog_expression{unquoted_atom{"y"}},
                    })},
                    .m_guide = prolog_expression{std::list<prolog_expression>({
                        prolog_expression{unquoted_atom{"bout"}},
                        prolog_expression{unquoted_atom{"daniel"}},
                        prolog_expression{std::list<prolog_expression>({
                            prolog_expression{unquoted_atom{"mp"}},
                            prolog_expression{std::list<prolog_expression>({
                                prolog_expression{unquoted_atom{"theorem"}},
                                prolog_expression{unquoted_atom{"a0"}},
                            })},
                            prolog_expression{std::list<prolog_expression>({
                                prolog_expression{unquoted_atom{"theorem"}},
                                prolog_expression{unquoted_atom{"a1"}},
                            })},
                        })},
                    })},
                },
            },
            {
                "infer 'i0' [claim daniel y] [bout daniel [dout daniel [mp [theorem a2] [theorem a3]]]]",
                infer_statement{
                    .m_tag = quoted_atom{"i0"},
                    .m_theorem = prolog_expression{std::list<prolog_expression>({
                        prolog_expression{unquoted_atom{"claim"}},
                        prolog_expression{unquoted_atom{"daniel"}},
                        prolog_expression{unquoted_atom{"y"}},
                    })},
                    .m_guide = prolog_expression{std::list<prolog_expression>({
                        prolog_expression{unquoted_atom{"bout"}},
                        prolog_expression{unquoted_atom{"daniel"}},
                        prolog_expression{std::list<prolog_expression>({
                            prolog_expression{unquoted_atom{"dout"}},
                            prolog_expression{unquoted_atom{"daniel"}},
                            prolog_expression{std::list<prolog_expression>({
                                prolog_expression{unquoted_atom{"mp"}},
                                prolog_expression{std::list<prolog_expression>({
                                    prolog_expression{unquoted_atom{"theorem"}},
                                    prolog_expression{unquoted_atom{"a2"}},
                                })},
                                prolog_expression{std::list<prolog_expression>({
                                    prolog_expression{unquoted_atom{"theorem"}},
                                    prolog_expression{unquoted_atom{"a3"}},
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
                    .m_theorem = prolog_expression{std::list<prolog_expression>({
                        prolog_expression{unquoted_atom{"claim"}},
                        prolog_expression{unquoted_atom{"daniel"}},
                        prolog_expression{unquoted_atom{"y"}},
                    })},
                    .m_guide = prolog_expression{unquoted_atom{"a0"}},
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
    using unilog::prolog_expression;
    using unilog::quoted_atom;
    using unilog::refer_statement;
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
    using unilog::prolog_expression;
    using unilog::quoted_atom;
    using unilog::refer_statement;
    using unilog::statement;
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
                                       .m_theorem = prolog_expression{unquoted_atom{"test"}},
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
    using unilog::prolog_expression;
    using unilog::quoted_atom;
    using unilog::refer_statement;
    using unilog::statement;
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
                                       .m_theorem = prolog_expression{std::list<prolog_expression>({
                                           prolog_expression{unquoted_atom{"if"}},
                                           prolog_expression{unquoted_atom{"y"}},
                                           prolog_expression{unquoted_atom{"x"}},
                                       })},
                                   },
                                   axiom_statement{
                                       .m_tag = unquoted_atom{"a1"},
                                       .m_theorem = prolog_expression{unquoted_atom{"x"}},
                                   },

                               }));
}

void test_parser_main()
{
    constexpr bool ENABLE_DEBUG_LOGS = true;

    TEST(test_parser_extract_prolog_expression);
    TEST(test_parser_extract_axiom_statement);
    TEST(test_parser_extract_guide_statement);
    TEST(test_parser_extract_infer_statement);
    TEST(test_parser_extract_refer_statement);
    TEST(test_parse_file_example_0);
    TEST(test_parse_file_example_1);
}
