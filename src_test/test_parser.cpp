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

#define LOG(x)             \
    if (ENABLE_DEBUG_LOGS) \
        std::cout << x;

#define TEST(void_fn) \
    void_fn();        \
    LOG("TEST COMPLETED: " << #void_fn << std::endl);

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
    using unilog::command;
    using unilog::lexeme;
    using unilog::list_close;
    using unilog::list_open;
    using unilog::prolog_expression;
    using unilog::variable;

    std::map<std::string, prolog_expression> l_test_cases =
        {
            {
                "if",
                prolog_expression{
                    atom{
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
                            atom{
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
                            atom{
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
                            atom{
                                "abc",
                            },
                        },
                        prolog_expression{
                            variable{
                                "Var",
                            },
                        },
                        prolog_expression{
                            atom{
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
                            atom{
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
                            atom{
                                "abc",
                            },
                        },
                        prolog_expression{
                            std::list<prolog_expression>{

                            },
                        },
                        prolog_expression{
                            atom{
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
                            atom{
                                "abc",
                            },
                        },
                        prolog_expression{
                            std::list<prolog_expression>{
                                prolog_expression{
                                    atom{
                                        "123",
                                    },
                                },
                            },
                        },
                        prolog_expression{
                            atom{
                                "def",
                            },
                        },
                    },
                },
            },
            {
                "abc []",
                prolog_expression{
                    atom{
                        "abc",
                    },
                },
            },
            {
                "[abc [123 [def [456 [ghi]]]]]",
                prolog_expression{
                    std::list<prolog_expression>{
                        prolog_expression{
                            atom{
                                "abc",
                            },
                        },
                        prolog_expression{
                            std::list<prolog_expression>{
                                prolog_expression{
                                    atom{
                                        "123",
                                    },
                                },
                                prolog_expression{
                                    std::list<prolog_expression>{
                                        prolog_expression{
                                            atom{
                                                "def",
                                            },
                                        },
                                        prolog_expression{
                                            std::list<prolog_expression>{
                                                prolog_expression{
                                                    atom{
                                                        "456",
                                                    },
                                                },
                                                prolog_expression{
                                                    std::list<prolog_expression>{
                                                        prolog_expression{
                                                            atom{
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
                            atom{
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
                                    atom{
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
                    atom{
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
                    atom{
                        "abc",
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

        LOG("success, case: \"" << l_key << "\"" << std::endl);
    }

    std::vector<std::string> l_expect_failure_inputs =
        {
            "[abc",
            "[[abc] [123]",
            "!axiom",
            "[!axiom]",
            "[!comm]",
            "]",
            // "abc]", // this is NOT an expect failure input.
            // this is because it will only try to parse the first prolog expression before a lexeme separator char.

        };

    for (const auto &l_input : l_expect_failure_inputs)
    {
        std::stringstream l_ss(l_input);

        prolog_expression l_exp;

        assert_throws(
            ([&l_ss, &l_exp]
             { l_ss >> l_exp; }));

        LOG("success, expected throw, case: " << l_input << std::endl);
    }
}

int test_parser_main()
{
    constexpr bool ENABLE_DEBUG_LOGS = true;

    TEST(test_parser_extract_prolog_expression);

    return 0;
}
