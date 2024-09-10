#include <iostream>
#include <sstream>
#include <assert.h>
#include <map>
#include <vector>
#include <iterator>
#include <algorithm>
#include <random>
#include "test_utils.hpp"

#include "test_utils.hpp"
#include "../src_lib/lexer.hpp"

// Function signatures to test
std::istream &escape(std::istream &a_istream, char &a_char);
std::istream &unilog::operator>>(std::istream &a_istream, unilog::lexeme &a_lexeme);

////////////////////////////////
//// HELPER FUNCTIONS
////////////////////////////////

////////////////////////////////
//// TESTS
////////////////////////////////

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

        escape(l_ss, l_escaped_char);

        // expect failure state of extraction
        assert(l_ss.fail());

        LOG("success, expected throw, case: " << l_input << std::endl);
    }
}

void test_lexer_extract_lexeme()
{
    constexpr bool ENABLE_DEBUG_LOGS = true;

    using unilog::atom;
    using unilog::command;
    using unilog::lexeme;
    using unilog::list_close;
    using unilog::list_open;
    using unilog::quoted_atom;
    using unilog::unquoted_atom;
    using unilog::variable;

    // Cases where content should be left alone:
    std::map<std::string, std::vector<lexeme>> l_desired_map = {
        {
            // Single command lexeme
            "!test",
            std::vector<lexeme>{
                command{
                    .m_text = "test",
                },
            },
        },
        {
            // Multiple command lexeme
            "!test1 !test2",
            std::vector<lexeme>{
                command{
                    .m_text = "test1",
                },
                command{
                    .m_text = "test2",
                },
            },
        },
        {
            // Single unquoted atom
            "test",
            std::vector<lexeme>{
                unquoted_atom{
                    .m_text = "test",
                },
            },
        },
        {
            // Command then unquoted atom
            "!test1 test2",
            std::vector<lexeme>{
                command{
                    .m_text = "test1",
                },
                unquoted_atom{
                    .m_text = "test2",
                },
            },
        },
        {
            // atom followed by command
            "test1 !test2",
            std::vector<lexeme>{
                unquoted_atom{
                    .m_text = "test1",
                },
                command{
                    .m_text = "test2",
                },
            },
        },
        {
            // Multiple atoms all unquoted
            "test 123",
            std::vector<lexeme>{
                unquoted_atom{
                    .m_text = "test",
                },
                unquoted_atom{
                    .m_text = "123",
                },
            },
        },
        {
            // space escape fails outside quotes
            "test\\ 123",
            std::vector<lexeme>{
                unquoted_atom{
                    .m_text = "test\\",
                },
                unquoted_atom{
                    .m_text = "123",
                },
            },
        },
        {
            // testing multiple failed escape sequences
            "test\\ 123\\ abc",
            std::vector<lexeme>{
                unquoted_atom{
                    .m_text = "test\\",
                },
                unquoted_atom{
                    .m_text = "123\\",
                },
                unquoted_atom{
                    .m_text = "abc",
                },
            },
        },
        {
            // three atoms, with irregular whitespace
            "test\\\t123\\\nabc",
            std::vector<lexeme>{
                unquoted_atom{
                    .m_text = "test\\",
                },
                unquoted_atom{
                    .m_text = "123\\",
                },
                unquoted_atom{
                    .m_text = "abc",
                },
            },
        },
        {
            // Multiple atoms unquoted
            "test 123 456",
            std::vector<lexeme>{
                unquoted_atom{
                    .m_text = "test",
                },
                unquoted_atom{
                    .m_text = "123",
                },
                unquoted_atom{
                    .m_text = "456",
                },
            },
        },
        {
            // Single unquoted atom
            "test_",
            std::vector<lexeme>{
                unquoted_atom{
                    .m_text = "test_",
                },
            },
        },
        {
            // Test single variable
            "_test",
            std::vector<lexeme>{
                variable{
                    .m_identifier = "_test",
                },
            },
        },
        {
            // Test single variable
            "Test_",
            std::vector<lexeme>{
                variable{
                    .m_identifier = "Test_",
                },
            },
        },
        {
            // Test single variable
            "Test",
            std::vector<lexeme>{
                variable{
                    .m_identifier = "Test",
                },
            },
        },
        {
            // Test 2 lexeme variables
            "Test1 Test2",
            std::vector<lexeme>{
                variable{
                    .m_identifier = "Test1",
                },
                variable{
                    .m_identifier = "Test2",
                },
            },
        },
        {
            // Test unnamed variable
            "_",
            std::vector<lexeme>{
                variable{
                    .m_identifier = "_",
                },
            },
        },
        {
            // Test single quoted atom
            "\'test\'",
            std::vector<lexeme>{
                quoted_atom{
                    .m_text = "test",
                },
            },
        },
        {
            // Test single lexeme quote
            "\"test\"",
            std::vector<lexeme>{
                quoted_atom{
                    .m_text = "test",
                },
            },
        },
        {
            // Test lexeme with both quotation types
            "\"test \' test\"",
            std::vector<lexeme>{
                quoted_atom{
                    .m_text = "test \' test",
                },
            },
        },
        {
            // Test lexeme with both quotation types
            "\'test \" test\'",
            std::vector<lexeme>{
                quoted_atom{
                    .m_text = "test \" test",
                },
            },
        },
        {
            // Test lexeme with single quotation type
            "\'test \' test \'\'",
            std::vector<lexeme>{
                quoted_atom{
                    .m_text = "test ",
                },
                unquoted_atom{
                    .m_text = "test",
                },
                quoted_atom{
                    .m_text = "",
                },
            },
        },
        {
            // Test single lexeme quote WITH UPPERCASE
            "\'Test\'",
            std::vector<lexeme>{
                quoted_atom{
                    .m_text = "Test",
                },
            },
        },
        {
            // Test spaces in quote
            "\'test blah blah\'",
            std::vector<lexeme>{
                quoted_atom{
                    .m_text = "test blah blah",
                },
            },
        },
        {
            // Test spaces BETWEEN quotes
            "\'test\' \'blah\' \'blah\'",
            std::vector<lexeme>{
                quoted_atom{
                    .m_text = "test",
                },
                quoted_atom{
                    .m_text = "blah",
                },
                quoted_atom{
                    .m_text = "blah",
                },
            },
        },
        {
            // Test embedding a backslash by escaping with another backslash
            "\'test\\\\\'",
            std::vector<lexeme>{
                quoted_atom{
                    .m_text = "test\\",
                },
            },
        },
        {
            // Test embedding a \n by escape sequence
            "\'test\\n\'",
            std::vector<lexeme>{
                quoted_atom{
                    .m_text = "test\n",
                },
            },
        },
        {
            // Test embedding a hex escape sequence
            "\'test\\x12\'",
            std::vector<lexeme>{
                quoted_atom{
                    .m_text = "test\x12",
                },
            },
        },
        {
            // Test embedding a hex escape sequence
            "\'test\\x123\'",
            std::vector<lexeme>{
                quoted_atom{
                    .m_text = "test\x12"
                              "3",
                },
            },
        },
        {
            // Test quoted escape sequence, followed by unquoted text
            "\'test\\x12\' test12",
            std::vector<lexeme>{
                quoted_atom{
                    .m_text = "test\x12",
                },
                unquoted_atom{
                    .m_text = "test12",
                },
            },
        },
        {
            // Test list open
            "[",
            std::vector<lexeme>{
                list_open{},
            },
        },
        {
            // Test list close
            "]",
            std::vector<lexeme>{
                list_close{},
            },
        },
        {
            // Test list open and list close
            "[]",
            std::vector<lexeme>{
                list_open{},
                list_close{},
            },
        },
        {
            // Test 2 empty lists with no separation
            "[][]",
            std::vector<lexeme>{
                list_open{},
                list_close{},
                list_open{},
                list_close{},
            },
        },
        {
            // Test 2 empty lists with separation
            "[] []",
            std::vector<lexeme>{
                list_open{},
                list_close{},
                list_open{},
                list_close{},
            },
        },
        {
            // Test flat list with 1 unquoted text
            "[abc]",
            std::vector<lexeme>{
                list_open{},
                unquoted_atom{
                    .m_text = "abc",
                },
                list_close{},
            },
        },
        {
            // Test 2 flat lists side-by-side
            "[abc][]",
            std::vector<lexeme>{
                list_open{},
                unquoted_atom{
                    .m_text = "abc",
                },
                list_close{},
                list_open{},
                list_close{},
            },
        },
        {
            // Test 2 flat lists side-by-side
            "[abc] [def]",
            std::vector<lexeme>{
                list_open{},
                unquoted_atom{
                    .m_text = "abc",
                },
                list_close{},
                list_open{},
                unquoted_atom{
                    .m_text = "def",
                },
                list_close{},
            },
        },
        {
            // Test 2 lists, one nested with unquoted text, with innermost list empty
            "[abc] [def []]",
            std::vector<lexeme>{
                list_open{},
                unquoted_atom{
                    .m_text = "abc",
                },
                list_close{},
                list_open{},
                unquoted_atom{
                    .m_text = "def",
                },
                list_open{},
                list_close{},
                list_close{},
            },
        },
        {
            // Test 1 list, containing variable
            "[A]",
            std::vector<lexeme>{
                list_open{},
                variable{
                    .m_identifier = "A",
                },
                list_close{},
            },
        },
        {
            // Test 1 list, containing variable and 1 variable immediately outside without separation
            "[A]B",
            std::vector<lexeme>{
                list_open{},
                variable{
                    .m_identifier = "A",
                },
                list_close{},
                variable{
                    .m_identifier = "B",
                },
            },
        },
        {
            // Test 1 list, containing 2 variables
            "[A B]",
            std::vector<lexeme>{
                list_open{},
                variable{
                    .m_identifier = "A",
                },
                variable{
                    .m_identifier = "B",
                },
                list_close{},
            },
        },
        {
            // Test 1 list, containing 2 variables and 1 atom
            "[A B abc]",
            std::vector<lexeme>{
                list_open{},
                variable{
                    .m_identifier = "A",
                },
                variable{
                    .m_identifier = "B",
                },
                unquoted_atom{
                    .m_text = "abc",
                },
                list_close{},
            },
        },
        {
            // Test 1 list, containing 2 variables and 1 atom
            "[_ _ abc]",
            std::vector<lexeme>{
                list_open{},
                variable{
                    .m_identifier = "_",
                },
                variable{
                    .m_identifier = "_",
                },
                unquoted_atom{
                    .m_text = "abc",
                },
                list_close{},
            },
        },
        {
            // Test composite list containing atoms and varaibles, some unnamed
            "[abc [_ _] [def A]]",
            std::vector<lexeme>{
                list_open{},
                unquoted_atom{
                    .m_text = "abc",
                },
                list_open{},
                variable{
                    .m_identifier = "_",
                },
                variable{
                    .m_identifier = "_",
                },
                list_close{},
                list_open{},
                unquoted_atom{
                    .m_text = "def",
                },
                variable{
                    .m_identifier = "A",
                },
                list_close{},
                list_close{},
            },
        },
        {
            // Test unconventional white-space
            "abc\ndef\nA\n\t_",
            std::vector<lexeme>{
                unquoted_atom{
                    .m_text = "abc",
                },
                unquoted_atom{
                    .m_text = "def",
                },
                variable{
                    .m_identifier = "A",
                },
                variable{
                    .m_identifier = "_",
                },
            },
        },
        {
            // Test unconventional white-space
            "abc\t[\ndef\nA\n\t_",
            std::vector<lexeme>{
                unquoted_atom{
                    .m_text = "abc",
                },
                list_open{},
                unquoted_atom{
                    .m_text = "def",
                },
                variable{
                    .m_identifier = "A",
                },
                variable{
                    .m_identifier = "_",
                },
            },
        },
        {
            // Test unconventional white-space
            "abc\t[\n\'\\n \\t \\\\\'\nA\n\t_",
            std::vector<lexeme>{
                unquoted_atom{
                    .m_text = "abc",
                },
                list_open{},
                quoted_atom{
                    .m_text = "\n \t \\",
                },
                variable{
                    .m_identifier = "A",
                },
                variable{
                    .m_identifier = "_",
                },
            },
        },
        {
            // Single atom, failing to escape the interpreter's separation of lexemes
            "test\\[",
            std::vector<lexeme>{
                unquoted_atom{
                    .m_text = "test\\",
                },
                list_open{},
            },
        },
        {
            // Single atom, failing to escape the interpreter's separation of lexemes
            "test\\xa4\\[",
            std::vector<lexeme>{
                unquoted_atom{
                    .m_text = "test\\xa4\\",
                },
                list_open{},
            },
        },
        {
            // single atom, without separation of command indicator
            "test!@.$^&*()",
            std::vector<lexeme>{
                unquoted_atom{
                    .m_text = "test",
                },
                command{
                    .m_text = "",
                },
                unquoted_atom{
                    .m_text = "@.$^&*()",
                },
            },
        },
        {
            // Single atom, with special characters
            "1.24",
            std::vector<lexeme>{
                unquoted_atom{
                    .m_text = "1.24",
                },
            },
        },
        {
            // Single atom, with special characters
            "[[[1.24]]]",
            std::vector<lexeme>{
                list_open{},
                list_open{},
                list_open{},
                unquoted_atom{
                    .m_text = "1.24",
                },
                list_close{},
                list_close{},
                list_close{},
            },
        },
        {
            // Single atom, with special characters
            "@",
            std::vector<lexeme>{
                unquoted_atom{
                    .m_text = "@",
                },
            },
        },
        {
            // Single atom, with special characters
            "+",
            std::vector<lexeme>{
                unquoted_atom{
                    .m_text = "+",
                },
            },
        },
        {
            // Single atom, with special characters
            "<-",
            std::vector<lexeme>{
                unquoted_atom{
                    .m_text = "<-",
                },
            },
        },
        {
            // Having fun, realistic scenario
            "!axiom add_bc_0 [add [] L L]",
            std::vector<lexeme>{
                command{
                    .m_text = "axiom",
                },
                unquoted_atom{
                    .m_text = "add_bc_0",
                },
                list_open{},
                unquoted_atom{
                    .m_text = "add",
                },
                list_open{},
                list_close{},
                variable{
                    .m_identifier = "L",
                },
                variable{
                    .m_identifier = "L",
                },
                list_close{},
            },
        },
        {
            // Having fun, realistic scenario
            "!axiom add_gc\n"
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
            std::vector<lexeme>{
                command{
                    .m_text = "axiom",
                },
                unquoted_atom{
                    .m_text = "add_gc",
                },
                list_open{},
                unquoted_atom{
                    .m_text = "if",
                },
                list_open{},
                unquoted_atom{
                    .m_text = "add",
                },
                variable{
                    .m_identifier = "X",
                },
                variable{
                    .m_identifier = "Y",
                },
                variable{
                    .m_identifier = "Z",
                },
                list_close{},
                list_open{},
                unquoted_atom{
                    .m_text = "and",
                },

                list_open{},
                unquoted_atom{
                    .m_text = "cons",
                },
                variable{
                    .m_identifier = "X",
                },
                variable{
                    .m_identifier = "XH",
                },
                variable{
                    .m_identifier = "XT",
                },
                list_close{},

                list_open{},
                unquoted_atom{
                    .m_text = "cons",
                },
                variable{
                    .m_identifier = "Y",
                },
                variable{
                    .m_identifier = "YH",
                },
                variable{
                    .m_identifier = "YT",
                },
                list_close{},

                list_open{},
                unquoted_atom{
                    .m_text = "add",
                },

                list_open{},
                variable{
                    .m_identifier = "XH",
                },
                list_close{},

                list_open{},
                variable{
                    .m_identifier = "YH",
                },
                list_close{},

                list_open{},
                variable{
                    .m_identifier = "ZH",
                },
                list_close{},

                list_close{},

                list_open{},
                unquoted_atom{
                    .m_text = "add",
                },
                variable{
                    .m_identifier = "XT",
                },
                variable{
                    .m_identifier = "YT",
                },
                variable{
                    .m_identifier = "ZT",
                },
                list_close{},

                list_open{},
                unquoted_atom{
                    .m_text = "cons",
                },
                variable{
                    .m_identifier = "Z",
                },
                variable{
                    .m_identifier = "ZH",
                },
                variable{
                    .m_identifier = "ZT",
                },
                list_close{},

                list_close{},
                list_close{},

            },
        },
        {
            // Single command lexeme
            "!test1[abc]",
            std::vector<lexeme>{
                command{
                    .m_text = "test1",
                },
                list_open{},
                unquoted_atom{
                    .m_text = "abc",
                },
                list_close{},
            },
        },
        {
            // Single command lexeme
            "[!test1][abc]",
            std::vector<lexeme>{
                list_open{},
                command{
                    .m_text = "test1",
                },
                list_close{},
                list_open{},
                unquoted_atom{
                    .m_text = "abc",
                },
                list_close{},
            },
        },
        {
            // Test title-case command
            "!Test",
            std::vector<lexeme>{
                command{
                    .m_text = "Test",
                },
            },
        },
        {
            // Test lex stop character: whitespace
            "1.24 ",
            std::vector<lexeme>{
                unquoted_atom{
                    .m_text = "1.24",
                },
            },
        },
        {
            // Test lex stop character: eof
            "1.24",
            std::vector<lexeme>{
                unquoted_atom{
                    .m_text = "1.24",
                },
            },
        },
        {
            // Test lex stop character: list open
            "1.24[",
            std::vector<lexeme>{
                unquoted_atom{
                    .m_text = "1.24",
                },
                list_open{},
            },
        },
        {
            // Test lex stop character: list close
            "1.24]",
            std::vector<lexeme>{
                unquoted_atom{
                    .m_text = "1.24",
                },
                list_close{},
            },
        },
        {
            "Test@",
            std::vector<lexeme>{
                variable{
                    "Test",
                },
                unquoted_atom{
                    "@",
                },
            },
        },
        {
            "Test#",
            std::vector<lexeme>{
                variable{
                    "Test",
                },
                unquoted_atom{
                    "#",
                },
            },
        },
        {
            "_#",
            std::vector<lexeme>{
                variable{
                    "_",
                },
                unquoted_atom{
                    "#",
                },
            },
        },
        {
            "Test1 Test_ Test_#",
            std::vector<lexeme>{
                variable{
                    "Test1",
                },
                variable{
                    "Test_",
                },
                variable{
                    "Test_",
                },
                unquoted_atom{
                    "#",
                },
            },
        },
        {
            "!tes@t",
            std::vector<lexeme>{
                command{
                    "tes",
                },
                unquoted_atom{
                    "@t",
                },
            },
        },
        {
            "!test1\\!test2",
            std::vector<lexeme>{
                command{
                    "test1",
                },
                unquoted_atom{
                    "\\",
                },
                command{
                    "test2",
                },
            },
        },
        {
            "!test1!test2",
            std::vector<lexeme>{
                command{
                    "test1",
                },
                command{
                    "test2",
                },
            },
        },
    };

    // Execute pre-made tests
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

    std::vector<std::string> l_expect_failure_inputs =
        {
            "\'hello, this is an unclosed quote",
            "\"hello, this is an unclosed quote",
        };

    for (const auto &l_input : l_expect_failure_inputs)
    {
        std::stringstream l_ss(l_input);

        lexeme l_lexeme;

        l_ss >> l_lexeme;

        // Make sure the stream state has its failbit set.
        assert(l_ss.fail());

        LOG("success, expected stream fail state, case: " << l_input << std::endl);
    }
}

void test_lexer_main()
{
    constexpr bool ENABLE_DEBUG_LOGS = true;

    TEST(test_lexer_escape);
    TEST(test_lexer_extract_lexeme);

    return;
}
