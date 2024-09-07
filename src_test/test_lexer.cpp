#include <iostream>
#include <sstream>
#include <assert.h>
#include <map>
#include <vector>
#include <iterator>
#include <algorithm>
#include "test_utils.hpp"

#include "../src_lib/lexer.hpp"

#define LOG(x)             \
    if (ENABLE_DEBUG_LOGS) \
        std::cout << x;

#define TEST(void_fn) \
    void_fn();        \
    LOG("TEST COMPLETED: " << #void_fn << std::endl);

namespace unilog
{
    int fxn();
}

// Function signatures to test
std::istream &escape(std::istream &a_istream, char &a_char);
std::istream &unilog::operator>>(std::istream &a_istream, unilog::lexeme &a_lexeme);

void test_lexer_escape()
{
    constexpr bool ENABLE_DEBUG_LOGS = true;

    // Cases where content should be left alone:
    std::map<std::string, char> l_desired_map = {
        {"0", '\0'},
        {"a", '\a'},
        {"b", '\b'},
        {"t", '\t'},
        {"n", '\n'},
        {"v", '\v'},
        {"f", '\f'},
        {"r", '\r'},
        {"x00", 0},
        {"x01", 1},
        {"x02", 2},
        {"x10", 16},
        {"xFF", 255},
        {"xFf", 255},
        {"xff", 255},
        {"xAb", 0xAB},
        {"xaB", 0xAB},
        {"c", 'c'},
        {"\'", '\''},
        {"\\", '\\'},
        {"(", '('},
        {")", ')'},
        {"[", '['},
        {"+", '+'},
    };

    for (const auto &[l_key, l_value] : l_desired_map)
    {
        std::stringstream l_ss(l_key);

        char l_escaped_char;
        escape(l_ss, l_escaped_char);

        // Ensure the escape routine succeeded on this test case.
        assert(l_escaped_char == l_value);

        // Ensure the stream content was actually consumed
        assert((size_t)l_ss.tellg() == l_key.size());

        LOG("success, case: " << l_key << std::endl);
    }

    std::vector<std::string> l_expect_failure_inputs =
        {
            "x0",
            "x",
            "x1Z",
            "x1z",
            "xg0",
        };

    for (const auto &l_input : l_expect_failure_inputs)
    {
        std::stringstream l_ss(l_input);

        char l_escaped_char;

        assert_throws(
            ([&l_ss, &l_escaped_char]
             { escape(l_ss, l_escaped_char); }));

        LOG("success, expected throw, case: " << l_input << std::endl);
    }
}

void test_lexer_extract_lexeme()
{
    constexpr bool ENABLE_DEBUG_LOGS = true;

    using unilog::lexeme;
    using unilog::token_types;

    // Cases where content should be left alone:
    std::map<std::string, std::vector<lexeme>> l_desired_map = {
        {
            // Single unquoted lexeme
            "test",
            std::vector{
                lexeme{
                    .m_token_type = token_types::atom,
                    .m_token_text = "test",
                },
            },
        },
        {
            // Multiple lexemes all unquoted
            "test 123",
            std::vector{
                lexeme{
                    .m_token_type = token_types::atom,
                    .m_token_text = "test",
                },
                lexeme{
                    .m_token_type = token_types::atom,
                    .m_token_text = "123",
                },
            },
        },
        {
            // One lexeme, space escaped
            "test\\ 123",
            std::vector{
                lexeme{
                    .m_token_type = token_types::atom,
                    .m_token_text = "test 123",
                },
            },
        },
        {
            // One lexeme, space escaped
            "test\\ 123\\ abc",
            std::vector{
                lexeme{
                    .m_token_type = token_types::atom,
                    .m_token_text = "test 123 abc",
                },
            },
        },
        {
            // One lexeme, space escaped
            "test\\\t123\\\nabc",
            std::vector{
                lexeme{
                    .m_token_type = token_types::atom,
                    .m_token_text = "test\t123\nabc",
                },
            },
        },
        {
            // Multiple lexemes all unquoted
            "test 123 456",
            std::vector{
                lexeme{
                    .m_token_type = token_types::atom,
                    .m_token_text = "test",
                },
                lexeme{
                    .m_token_type = token_types::atom,
                    .m_token_text = "123",
                },
                lexeme{
                    .m_token_type = token_types::atom,
                    .m_token_text = "456",
                },
            },
        },
        {
            // Single unquoted lexeme
            "test_",
            std::vector{
                lexeme{
                    .m_token_type = token_types::atom,
                    .m_token_text = "test_",
                },
            },
        },
        {
            // Test single lexeme variable
            "_test",
            std::vector{
                lexeme{
                    .m_token_type = token_types::variable,
                    .m_token_text = "_test",
                },
            },
        },
        {
            // Test single lexeme variable
            "Test",
            std::vector{
                lexeme{
                    .m_token_type = token_types::variable,
                    .m_token_text = "Test",
                },
            },
        },
        {
            // Test 2 lexeme variables
            "Test1 Test2",
            std::vector{
                lexeme{
                    .m_token_type = token_types::variable,
                    .m_token_text = "Test1",
                },
                lexeme{
                    .m_token_type = token_types::variable,
                    .m_token_text = "Test2",
                },
            },
        },
        {
            // Test unnamed variable
            "_",
            std::vector{
                lexeme{
                    .m_token_type = token_types::variable,
                    .m_token_text = "_",
                },
            },
        },
        {
            // Test single lexeme quote
            "\'test\'",
            std::vector{
                lexeme{
                    .m_token_type = token_types::atom,
                    .m_token_text = "test",
                },
            },
        },
        {
            // Test single lexeme quote WITH UPPERCASE
            "\'Test\'",
            std::vector{
                lexeme{
                    .m_token_type = token_types::atom,
                    .m_token_text = "Test",
                },
            },
        },
        {
            // Test spaces in quote
            "\'test blah blah\'",
            std::vector{
                lexeme{
                    .m_token_type = token_types::atom,
                    .m_token_text = "test blah blah",
                },
            },
        },
        {
            // Test spaces BETWEEN quotes
            "\'test\' \'blah\' \'blah\'",
            std::vector{
                lexeme{
                    .m_token_type = token_types::atom,
                    .m_token_text = "test",
                },
                lexeme{
                    .m_token_type = token_types::atom,
                    .m_token_text = "blah",
                },
                lexeme{
                    .m_token_type = token_types::atom,
                    .m_token_text = "blah",
                },
            },
        },
        {
            // Test embedding a backslash by escaping with another backslash
            "\'test\\\\\'",
            std::vector{
                lexeme{
                    .m_token_type = token_types::atom,
                    .m_token_text = "test\\",
                },
            },
        },
        {
            // Test embedding a \n by escape sequence
            "\'test\\n\'",
            std::vector{
                lexeme{
                    .m_token_type = token_types::atom,
                    .m_token_text = "test\n",
                },
            },
        },
        {
            // Test embedding a hex escape sequence
            "\'test\\x12\'",
            std::vector{
                lexeme{
                    .m_token_type = token_types::atom,
                    .m_token_text = "test\x12",
                },
            },
        },
        {
            // Test embedding a hex escape sequence
            "\'test\\x123\'",
            std::vector{
                lexeme{
                    .m_token_type = token_types::atom,
                    .m_token_text = "test\x12"
                                    "3",
                },
            },
        },
        {
            // Test quoted escape sequence, followed by unquoted text
            "\'test\\x12\' test12",
            std::vector{
                lexeme{
                    .m_token_type = token_types::atom,
                    .m_token_text = "test\x12",
                },
                lexeme{
                    .m_token_type = token_types::atom,
                    .m_token_text = "test12",
                },
            },
        },
        {
            // Test list open
            "[",
            std::vector{
                lexeme{
                    .m_token_type = token_types::list_open,
                    .m_token_text = "[",
                },
            },
        },
        {
            // Test list close
            "]",
            std::vector{
                lexeme{
                    .m_token_type = token_types::list_close,
                    .m_token_text = "]",
                },
            },
        },
        {
            // Test list open and list close
            "[]",
            std::vector{
                lexeme{
                    .m_token_type = token_types::list_open,
                    .m_token_text = "[",
                },
                lexeme{
                    .m_token_type = token_types::list_close,
                    .m_token_text = "]",
                },
            },
        },
        {
            // Test 2 empty lists with no separation
            "[][]",
            std::vector{
                lexeme{
                    .m_token_type = token_types::list_open,
                    .m_token_text = "[",
                },
                lexeme{
                    .m_token_type = token_types::list_close,
                    .m_token_text = "]",
                },
                lexeme{
                    .m_token_type = token_types::list_open,
                    .m_token_text = "[",
                },
                lexeme{
                    .m_token_type = token_types::list_close,
                    .m_token_text = "]",
                },
            },
        },
        {
            // Test 2 empty lists with separation
            "[] []",
            std::vector{
                lexeme{
                    .m_token_type = token_types::list_open,
                    .m_token_text = "[",
                },
                lexeme{
                    .m_token_type = token_types::list_close,
                    .m_token_text = "]",
                },
                lexeme{
                    .m_token_type = token_types::list_open,
                    .m_token_text = "[",
                },
                lexeme{
                    .m_token_type = token_types::list_close,
                    .m_token_text = "]",
                },
            },
        },
        {
            // Test flat list with 1 unquoted text
            "[abc]",
            std::vector{
                lexeme{
                    .m_token_type = token_types::list_open,
                    .m_token_text = "[",
                },
                lexeme{
                    .m_token_type = token_types::atom,
                    .m_token_text = "abc",
                },
                lexeme{
                    .m_token_type = token_types::list_close,
                    .m_token_text = "]",
                },
            },
        },
        {
            // Test 2 flat lists side-by-side
            "[abc][]",
            std::vector{
                lexeme{
                    .m_token_type = token_types::list_open,
                    .m_token_text = "[",
                },
                lexeme{
                    .m_token_type = token_types::atom,
                    .m_token_text = "abc",
                },
                lexeme{
                    .m_token_type = token_types::list_close,
                    .m_token_text = "]",
                },
                lexeme{
                    .m_token_type = token_types::list_open,
                    .m_token_text = "[",
                },
                lexeme{
                    .m_token_type = token_types::list_close,
                    .m_token_text = "]",
                },
            },
        },
        {
            // Test 2 flat lists side-by-side
            "[abc] [def]",
            std::vector{
                lexeme{
                    .m_token_type = token_types::list_open,
                    .m_token_text = "[",
                },
                lexeme{
                    .m_token_type = token_types::atom,
                    .m_token_text = "abc",
                },
                lexeme{
                    .m_token_type = token_types::list_close,
                    .m_token_text = "]",
                },
                lexeme{
                    .m_token_type = token_types::list_open,
                    .m_token_text = "[",
                },
                lexeme{
                    .m_token_type = token_types::atom,
                    .m_token_text = "def",
                },
                lexeme{
                    .m_token_type = token_types::list_close,
                    .m_token_text = "]",
                },
            },
        },
        {
            // Test 2 lists, one nested with unquoted text, with innermost list empty
            "[abc] [def []]",
            std::vector{
                lexeme{
                    .m_token_type = token_types::list_open,
                    .m_token_text = "[",
                },
                lexeme{
                    .m_token_type = token_types::atom,
                    .m_token_text = "abc",
                },
                lexeme{
                    .m_token_type = token_types::list_close,
                    .m_token_text = "]",
                },
                lexeme{
                    .m_token_type = token_types::list_open,
                    .m_token_text = "[",
                },
                lexeme{
                    .m_token_type = token_types::atom,
                    .m_token_text = "def",
                },
                lexeme{
                    .m_token_type = token_types::list_open,
                    .m_token_text = "[",
                },
                lexeme{
                    .m_token_type = token_types::list_close,
                    .m_token_text = "]",
                },
                lexeme{
                    .m_token_type = token_types::list_close,
                    .m_token_text = "]",
                },
            },
        },
        {
            // Test 1 list, containing variable
            "[A]",
            std::vector{
                lexeme{
                    .m_token_type = token_types::list_open,
                    .m_token_text = "[",
                },
                lexeme{
                    .m_token_type = token_types::variable,
                    .m_token_text = "A",
                },
                lexeme{
                    .m_token_type = token_types::list_close,
                    .m_token_text = "]",
                },
            },
        },
        {
            // Test 1 list, containing variable and 1 variable immediately outside without separation
            "[A]B",
            std::vector{
                lexeme{
                    .m_token_type = token_types::list_open,
                    .m_token_text = "[",
                },
                lexeme{
                    .m_token_type = token_types::variable,
                    .m_token_text = "A",
                },
                lexeme{
                    .m_token_type = token_types::list_close,
                    .m_token_text = "]",
                },
                lexeme{
                    .m_token_type = token_types::variable,
                    .m_token_text = "B",
                },
            },
        },
        {
            // Test 1 list, containing 2 variables
            "[A B]",
            std::vector{
                lexeme{
                    .m_token_type = token_types::list_open,
                    .m_token_text = "[",
                },
                lexeme{
                    .m_token_type = token_types::variable,
                    .m_token_text = "A",
                },
                lexeme{
                    .m_token_type = token_types::variable,
                    .m_token_text = "B",
                },
                lexeme{
                    .m_token_type = token_types::list_close,
                    .m_token_text = "]",
                },
            },
        },
        {
            // Test 1 list, containing 2 variables and 1 atom
            "[A B abc]",
            std::vector{
                lexeme{
                    .m_token_type = token_types::list_open,
                    .m_token_text = "[",
                },
                lexeme{
                    .m_token_type = token_types::variable,
                    .m_token_text = "A",
                },
                lexeme{
                    .m_token_type = token_types::variable,
                    .m_token_text = "B",
                },
                lexeme{
                    .m_token_type = token_types::atom,
                    .m_token_text = "abc",
                },
                lexeme{
                    .m_token_type = token_types::list_close,
                    .m_token_text = "]",
                },
            },
        },
        {
            // Test 1 list, containing 2 variables and 1 atom
            "[_ _ abc]",
            std::vector{
                lexeme{
                    .m_token_type = token_types::list_open,
                    .m_token_text = "[",
                },
                lexeme{
                    .m_token_type = token_types::variable,
                    .m_token_text = "_",
                },
                lexeme{
                    .m_token_type = token_types::variable,
                    .m_token_text = "_",
                },
                lexeme{
                    .m_token_type = token_types::atom,
                    .m_token_text = "abc",
                },
                lexeme{
                    .m_token_type = token_types::list_close,
                    .m_token_text = "]",
                },
            },
        },
        {
            // Test composite list containing atoms and varaibles, some unnamed
            "[abc [_ _] [def A]]",
            std::vector{
                lexeme{
                    .m_token_type = token_types::list_open,
                    .m_token_text = "[",
                },
                lexeme{
                    .m_token_type = token_types::atom,
                    .m_token_text = "abc",
                },
                lexeme{
                    .m_token_type = token_types::list_open,
                    .m_token_text = "[",
                },
                lexeme{
                    .m_token_type = token_types::variable,
                    .m_token_text = "_",
                },
                lexeme{
                    .m_token_type = token_types::variable,
                    .m_token_text = "_",
                },
                lexeme{
                    .m_token_type = token_types::list_close,
                    .m_token_text = "]",
                },
                lexeme{
                    .m_token_type = token_types::list_open,
                    .m_token_text = "[",
                },
                lexeme{
                    .m_token_type = token_types::atom,
                    .m_token_text = "def",
                },
                lexeme{
                    .m_token_type = token_types::variable,
                    .m_token_text = "A",
                },
                lexeme{
                    .m_token_type = token_types::list_close,
                    .m_token_text = "]",
                },
                lexeme{
                    .m_token_type = token_types::list_close,
                    .m_token_text = "]",
                },
            },
        },
        {
            // Test unconventional white-space
            "abc\ndef\nA\n\t_",
            std::vector{
                lexeme{
                    .m_token_type = token_types::atom,
                    .m_token_text = "abc",
                },
                lexeme{
                    .m_token_type = token_types::atom,
                    .m_token_text = "def",
                },
                lexeme{
                    .m_token_type = token_types::variable,
                    .m_token_text = "A",
                },
                lexeme{
                    .m_token_type = token_types::variable,
                    .m_token_text = "_",
                },
            },
        },
        {
            // Test unconventional white-space
            "abc\t[\ndef\nA\n\t_",
            std::vector{
                lexeme{
                    .m_token_type = token_types::atom,
                    .m_token_text = "abc",
                },
                lexeme{
                    .m_token_type = token_types::list_open,
                    .m_token_text = "[",
                },
                lexeme{
                    .m_token_type = token_types::atom,
                    .m_token_text = "def",
                },
                lexeme{
                    .m_token_type = token_types::variable,
                    .m_token_text = "A",
                },
                lexeme{
                    .m_token_type = token_types::variable,
                    .m_token_text = "_",
                },
            },
        },
        {
            // Test unconventional white-space
            "abc\t[\n\'\\n \\t \\\\\'\nA\n\t_",
            std::vector{
                lexeme{
                    .m_token_type = token_types::atom,
                    .m_token_text = "abc",
                },
                lexeme{
                    .m_token_type = token_types::list_open,
                    .m_token_text = "[",
                },
                lexeme{
                    .m_token_type = token_types::atom,
                    .m_token_text = "\n \t \\",
                },
                lexeme{
                    .m_token_type = token_types::variable,
                    .m_token_text = "A",
                },
                lexeme{
                    .m_token_type = token_types::variable,
                    .m_token_text = "_",
                },
            },
        },
        {
            // Single atom, escaping the interpreter's separation of lexemes
            "test\\[",
            std::vector{
                lexeme{
                    .m_token_type = token_types::atom,
                    .m_token_text = "test[",
                },
            },
        },
        {
            // Single atom, escaping the interpreter's separation of lexemes
            "test\\xa4\\[",
            std::vector{
                lexeme{
                    .m_token_type = token_types::atom,
                    .m_token_text = "test\xa4[",
                },
            },
        },
    };

    for (const auto &[l_key, l_value] : l_desired_map)
    {
        std::stringstream l_ss(l_key);

        std::vector<lexeme> l_lexemes;

        std::copy(std::istream_iterator<lexeme>(l_ss), std::istream_iterator<lexeme>(), std::back_inserter(l_lexemes));

        // Check to make sure the correct number of lexemes was extracted.
        assert(l_lexemes.size() == l_value.size());

        // Ensure the lexeme extraction succeeded
        assert(l_lexemes == l_value);

        LOG("success, case: \"" << l_key << "\"" << std::endl);
    }
}

int test_lexer_main()
{
    constexpr bool ENABLE_DEBUG_LOGS = true;

    std::cout << "linked: " << unilog::fxn() << std::endl;

    TEST(test_lexer_escape);
    TEST(test_lexer_extract_lexeme);

    return 0;
}
