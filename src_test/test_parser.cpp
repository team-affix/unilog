#include <iostream>
#include <sstream>
#include <assert.h>
#include <map>
#include <vector>
#include <iterator>
#include <algorithm>
#include <random>
#include "test_utils.hpp"

#include "../src_lib/parser.hpp"
#include "../src_lib/variant_functions.hpp"

// Function signatures to test
std::istream &unilog::operator>>(std::istream &a_istream, unilog::prolog_expression &a_prolog_expression);

////////////////////////////////
//// HELPER FUNCTIONS
////////////////////////////////

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

    data_points<std::string, prolog_expression> l_test_cases =
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
                "guide g_add_bc\n"
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
        };

    for (const auto &l_input : l_fail_cases)
    {
        std::stringstream l_ss(l_input);

        guide_statement l_statement;

        l_ss >> l_statement;

        // ensure failure of extraction
        assert(l_ss.fail());

        LOG("success, case: expected failure extracting axiom_statement: " << l_input << std::endl);
    }
}

int test_parser_main()
{
    constexpr bool ENABLE_DEBUG_LOGS = true;

    TEST(test_parser_extract_prolog_expression);
    TEST(test_parser_extract_axiom_statement);
    TEST(test_parser_extract_guide_statement);

    return 0;
}
