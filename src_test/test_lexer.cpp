#include <iostream>
#include <sstream>
#include <assert.h>
#include <map>
#include <vector>
#include <iterator>
#include <algorithm>
#include <random>
#include <fstream>
#include <filesystem>

#include "test_utils.hpp"
#include "../src_lib/variant_functions.hpp"
#include "../src_lib/lexer.hpp"

// Function signatures to test
std::istream &escape(std::istream &a_istream, char &a_char);

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

void test_lexer_eol_equivalence()
{
    using unilog::eol;

    data_points<std::pair<eol, eol>, bool> l_desired =
        {
            {
                {
                    eol{},
                    eol{},
                },
                true,
            },
        };

    for (const auto &[l_key, l_value] : l_desired)
    {
        assert((l_key.first == l_key.second) == l_value);
    }
}

void test_lexer_list_separator_equivalence()
{
    using unilog::list_separator;

    data_points<std::pair<list_separator, list_separator>, bool> l_desired =
        {
            {
                {
                    list_separator{},
                    list_separator{},
                },
                true,
            },
        };

    for (const auto &[l_key, l_value] : l_desired)
    {
        assert((l_key.first == l_key.second) == l_value);
    }
}

void test_lexer_list_open_equivalence()
{
    using unilog::list_open;

    data_points<std::pair<list_open, list_open>, bool> l_desired =
        {
            {
                {
                    list_open{},
                    list_open{},
                },
                true,
            },
        };

    for (const auto &[l_key, l_value] : l_desired)
    {
        assert((l_key.first == l_key.second) == l_value);
    }
}

void test_lexer_list_close_equivalence()
{
    using unilog::list_close;

    data_points<std::pair<list_close, list_close>, bool> l_desired =
        {
            {
                {
                    list_close{},
                    list_close{},
                },
                true,
            },
        };

    for (const auto &[l_key, l_value] : l_desired)
    {
        assert((l_key.first == l_key.second) == l_value);
    }
}

void test_lexer_variable_equivalence()
{
    using unilog::variable;

    data_points<std::pair<variable, variable>, bool> l_desired =
        {
            {
                {
                    variable{
                        "abc",
                    },
                    variable{
                        "abc",
                    },
                },
                true,
            },
            {
                {
                    variable{
                        "abc",
                    },
                    variable{
                        "abc1",
                    },
                },
                false,
            },
            {
                {
                    variable{
                        "",
                    },
                    variable{
                        "",
                    },
                },
                true,
            },
            {
                {
                    variable{
                        "abc",
                    },
                    variable{
                        "",
                    },
                },
                false,
            },
            {
                {
                    variable{
                        "",
                    },
                    variable{
                        "abc",
                    },
                },
                false,
            },
        };

    for (const auto &[l_key, l_value] : l_desired)
    {
        assert((l_key.first == l_key.second) == l_value);
    }
}

void test_lexer_atom_equivalence()
{
    using unilog::atom;

    data_points<std::pair<atom, atom>, bool> l_desired =
        {
            {
                {
                    atom{
                        "abc",
                    },
                    atom{
                        "abc",
                    },
                },
                true,
            },
            {
                {
                    atom{
                        "abc",
                    },
                    atom{
                        "abc1",
                    },
                },
                false,
            },
            {
                {
                    atom{
                        "",
                    },
                    atom{
                        "",
                    },
                },
                true,
            },
            {
                {
                    atom{
                        "abc",
                    },
                    atom{
                        "",
                    },
                },
                false,
            },
            {
                {
                    atom{
                        "",
                    },
                    atom{
                        "abc",
                    },
                },
                false,
            },
        };

    for (const auto &[l_key, l_value] : l_desired)
    {
        assert((l_key.first == l_key.second) == l_value);
    }
}

void test_lexer_extract_lexeme()
{
    constexpr bool ENABLE_DEBUG_LOGS = true;

    using unilog::atom;
    using unilog::eol;
    using unilog::lexeme;
    using unilog::list_close;
    using unilog::list_open;
    using unilog::list_separator;
    using unilog::variable;

    // Cases where content should be left alone:
    data_points<std::string, std::vector<lexeme>> l_desired_map = {
        {
            "a|",
            std::vector<lexeme>{
                atom{
                    "a",
                },
                list_separator{},
            },
        },
        {
            "a|[]",
            std::vector<lexeme>{
                atom{
                    "a",
                },
                list_separator{},
                list_open{},
                list_close{},
            },
        },
        {
            "[a|[] b|[] c|[]]",
            std::vector<lexeme>{
                list_open{},
                atom{
                    "a",
                },
                list_separator{},
                list_open{},
                list_close{},
                atom{
                    "b",
                },
                list_separator{},
                list_open{},
                list_close{},
                atom{
                    "c",
                },
                list_separator{},
                list_open{},
                list_close{},
                list_close{},
            },
        },
        {
            // Single unquoted atom
            "test",
            std::vector<lexeme>{
                atom{
                    .m_text = "test",
                },
            },
        },
        {
            // Single unquoted atom
            "test_",
            std::vector<lexeme>{
                atom{
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
                atom{
                    .m_text = "test",
                },
            },
        },
        {
            // Test single lexeme quote
            "\"test\"",
            std::vector<lexeme>{
                atom{
                    .m_text = "test",
                },
            },
        },
        {
            // Test lexeme with both quotation types
            "\"test \' test\"",
            std::vector<lexeme>{
                atom{
                    .m_text = "test \' test",
                },
            },
        },
        {
            // Test lexeme with both quotation types
            "\'test \" test\'",
            std::vector<lexeme>{
                atom{
                    .m_text = "test \" test",
                },
            },
        },
        {
            // Test lexeme with single quotation type
            "\'test \' test \'\'",
            std::vector<lexeme>{
                atom{
                    .m_text = "test ",
                },
                atom{
                    .m_text = "test",
                },
                atom{
                    .m_text = "",
                },
            },
        },
        {
            // Test single lexeme quote WITH UPPERCASE
            "\'Test\'",
            std::vector<lexeme>{
                atom{
                    .m_text = "Test",
                },
            },
        },
        {
            // Test spaces in quote
            "\'test blah blah\'",
            std::vector<lexeme>{
                atom{
                    .m_text = "test blah blah",
                },
            },
        },
        {
            // Test spaces BETWEEN quotes
            "\'test\' \'blah\' \'blah\'",
            std::vector<lexeme>{
                atom{
                    .m_text = "test",
                },
                atom{
                    .m_text = "blah",
                },
                atom{
                    .m_text = "blah",
                },
            },
        },
        {
            // Test embedding a backslash by escaping with another backslash
            "\'test\\\\\'",
            std::vector<lexeme>{
                atom{
                    .m_text = "test\\",
                },
            },
        },
        {
            // Test embedding a \n by escape sequence
            "\'test\\n\'",
            std::vector<lexeme>{
                atom{
                    .m_text = "test\n",
                },
            },
        },
        {
            // Test embedding a hex escape sequence
            "\'test\\x12\'",
            std::vector<lexeme>{
                atom{
                    .m_text = "test\x12",
                },
            },
        },
        {
            // Test embedding a hex escape sequence
            "\'test\\x123\'",
            std::vector<lexeme>{
                atom{
                    .m_text = "test\x12"
                              "3",
                },
            },
        },
        {
            // Test quoted escape sequence, followed by unquoted text
            "\'test\\x12\' test12",
            std::vector<lexeme>{
                atom{
                    .m_text = "test\x12",
                },
                atom{
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
                atom{
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
                atom{
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
                atom{
                    .m_text = "abc",
                },
                list_close{},
                list_open{},
                atom{
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
                atom{
                    .m_text = "abc",
                },
                list_close{},
                list_open{},
                atom{
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
                atom{
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
                atom{
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
                atom{
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
                atom{
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
                atom{
                    .m_text = "abc",
                },
                atom{
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
                atom{
                    .m_text = "abc",
                },
                list_open{},
                atom{
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
                atom{
                    .m_text = "abc",
                },
                list_open{},
                atom{
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
            // Single atom, with special characters
            "[[['1.24']]]",
            std::vector<lexeme>{
                list_open{},
                list_open{},
                list_open{},
                atom{
                    .m_text = "1.24",
                },
                list_close{},
                list_close{},
                list_close{},
            },
        },
        {
            // Having fun, realistic scenario
            "axiom add_bc_0 [add [] L L]",
            std::vector<lexeme>{
                atom{
                    .m_text = "axiom",
                },
                atom{
                    .m_text = "add_bc_0",
                },
                list_open{},
                atom{
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
            "axiom add_gc\n"
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
                atom{
                    .m_text = "axiom",
                },
                atom{
                    .m_text = "add_gc",
                },
                list_open{},
                atom{
                    .m_text = "if",
                },
                list_open{},
                atom{
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
                atom{
                    .m_text = "and",
                },

                list_open{},
                atom{
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
                atom{
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
                atom{
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
                atom{
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
                atom{
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
            // Test lex stop character: whitespace
            "abc ",
            std::vector<lexeme>{
                atom{
                    .m_text = "abc",
                },
            },
        },
        {
            // Test lex stop character: eof
            "abc",
            std::vector<lexeme>{
                atom{
                    .m_text = "abc",
                },
            },
        },
        {
            // Test lex stop character: list open
            "abc[",
            std::vector<lexeme>{
                atom{
                    .m_text = "abc",
                },
                list_open{},
            },
        },
        {
            // Test lex stop character: list close
            "abc]",
            std::vector<lexeme>{
                atom{
                    .m_text = "abc",
                },
                list_close{},
            },
        },
        {
            "Test|",
            std::vector<lexeme>{
                variable{
                    "Test",
                },
                list_separator{

                },
            },
        },
        {
            "_ _",
            std::vector<lexeme>{
                variable{
                    "_",
                },
                variable{
                    "_",
                },
            },
        },
        {
            "tes_t",
            std::vector<lexeme>{
                atom{
                    "tes_t",
                },
            },
        },
        {
            "test;",
            std::vector<lexeme>{
                atom{"test"},
                eol{},
            },
        },
        {
            "test1;test2",
            std::vector<lexeme>{
                atom{"test1"},
                eol{},
                atom{"test2"},
            },
        },
        {
            "[test1 test2];",
            std::vector<lexeme>{
                list_open{},
                atom{"test1"},
                atom{"test2"},
                list_close{},
                eol{},
            },
        },
        {
            ";;;[test1; \"test2\"];",
            std::vector<lexeme>{
                eol{},
                eol{},
                eol{},
                list_open{},
                atom{"test1"},
                eol{},
                atom{"test2"},
                list_close{},
                eol{},
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
            "    ",
            "",
            "!test1!test2",
            "!test1\\!test2",

            "#Test",

            "!Test",
            "!test1[abc]",
            "<-",
            "+",
            "@",
            "1.24",
            "\\test",

            // atoms that start with numbers
            "1ab",

            // special symbols
            "!",
            "@",
            "#",
            "$",
            "%",
            "^",
            "&",
            "*",
            "(",
            ")",
            "-",
            "=",

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

void test_lex_file_example_0()
{
    using unilog::atom;
    using unilog::lexeme;
    using unilog::list_close;
    using unilog::list_open;
    using unilog::list_separator;
    using unilog::variable;

    std::stringstream l_ss;
    std::ifstream l_if("./src_test/example_unilog_files/lexer_example_0/main.ul");
    // std::cout << std::filesystem::current_path() << std::endl;
    // std::cout << "is_good: " << l_if.good() << std::endl;
    l_ss << l_if.rdbuf(); // read in contents of file
    // std::cout << l_ss.str() << std::endl;

    std::list<lexeme> l_lexemes;
    std::copy(std::istream_iterator<lexeme>(l_ss), std::istream_iterator<lexeme>(), std::back_inserter(l_lexemes));

    assert(l_ss.eof()); // there is no trailing newline.
    assert(l_lexemes == std::list<lexeme>({
                            atom{"axiom"},
                            atom{"a0"},
                            list_open{},
                            atom{"if"},
                            atom{"y"},
                            atom{"x"},
                            list_close{},
                            atom{"axiom"},
                            atom{"a1"},
                            atom{"x"},
                        }));
}

void test_lex_file_example_1()
{
    using unilog::atom;
    using unilog::lexeme;
    using unilog::list_close;
    using unilog::list_open;
    using unilog::list_separator;
    using unilog::variable;

    std::stringstream l_ss;
    std::ifstream l_if("./src_test/example_unilog_files/lexer_example_1/main.ul");
    // std::cout << std::filesystem::current_path() << std::endl;
    // std::cout << "is_good: " << l_if.good() << std::endl;
    l_ss << l_if.rdbuf(); // read in contents of file
    // std::cout << l_ss.str() << std::endl;

    std::list<lexeme> l_lexemes;
    std::copy(std::istream_iterator<lexeme>(l_ss), std::istream_iterator<lexeme>(), std::back_inserter(l_lexemes));

    assert(l_ss.eof()); // there is no trailing newline.
    assert(l_lexemes == std::list<lexeme>({
                            atom{"axiom"},
                            atom{"a0"},
                            atom{"test"},

                            atom{"axiom"},
                            list_open{},
                            atom{"a1"},
                            list_close{},
                            atom{"x"},

                            atom{"axiom"},
                            atom{"a2"},
                            list_open{},
                            atom{"x"},
                            list_close{},
                        }));
}

void test_lex_file_example_2()
{
    using unilog::atom;
    using unilog::lexeme;
    using unilog::list_close;
    using unilog::list_open;
    using unilog::list_separator;
    using unilog::variable;

    std::stringstream l_ss;
    std::ifstream l_if("./src_test/example_unilog_files/lexer_example_2/main.ul");
    // std::cout << std::filesystem::current_path() << std::endl;
    // std::cout << "is_good: " << l_if.good() << std::endl;
    l_ss << l_if.rdbuf(); // read in contents of file
    // std::cout << l_ss.str() << std::endl;

    std::list<lexeme> l_lexemes;
    std::copy(std::istream_iterator<lexeme>(l_ss), std::istream_iterator<lexeme>(), std::back_inserter(l_lexemes));

    assert(l_ss.eof()); // there is no trailing newline.
    assert(l_lexemes == std::list<lexeme>({
                            atom{"infer"},
                            atom{"i0"},
                            list_open{},
                            atom{"if"},
                            variable{"Y"},
                            list_open{},
                            atom{"and"},
                            list_open{},
                            atom{"if"},
                            variable{"Y"},
                            variable{"X"},
                            list_close{},
                            variable{"X"},
                            list_close{},
                            list_close{},

                            atom{"axiom"},
                            atom{"a0"},
                            list_open{},
                            atom{"if"},
                            atom{"b"},
                            atom{"a"},
                            list_close{},

                            atom{"axiom"},
                            atom{"a1"},
                            atom{"a"},
                        }));
}

void test_lex_file_example_3()
{
    using unilog::atom;
    using unilog::lexeme;
    using unilog::list_close;
    using unilog::list_open;
    using unilog::list_separator;
    using unilog::variable;

    std::stringstream l_ss;
    std::ifstream l_if("./src_test/example_unilog_files/lexer_example_3/main.ul");
    // std::cout << std::filesystem::current_path() << std::endl;
    // std::cout << "is_good: " << l_if.good() << std::endl;
    l_ss << l_if.rdbuf(); // read in contents of file
    // std::cout << l_ss.str() << std::endl;
    std::list<lexeme> l_lexemes;
    std::copy(std::istream_iterator<lexeme>(l_ss), std::istream_iterator<lexeme>(), std::back_inserter(l_lexemes));

    // assert(l_ss.eof()); // assert successful extraction
    // THE ABOVE ASSERTION WILL NOT WORK. THIS IS BECAUSE stringstream HAS A CUSTOM
    //     IMPLEMENTATION OF SETTING eofbit, AND NO CHARACTERS WERE EVER EXTRACTED BEFORE
    //     REACHING END OF BUFFER THUS eofbit WILL NOT BE SET.
    assert(l_lexemes == std::list<lexeme>({

                        }));
}

void test_lex_file_example_4()
{
    using unilog::atom;
    using unilog::eol;
    using unilog::lexeme;
    using unilog::list_close;
    using unilog::list_open;
    using unilog::list_separator;
    using unilog::variable;

    std::stringstream l_ss;
    std::ifstream l_if("./src_test/example_unilog_files/lexer_example_4/jake.u");
    // std::cout << std::filesystem::current_path() << std::endl;
    // std::cout << "is_good: " << l_if.good() << std::endl;
    l_ss << l_if.rdbuf(); // read in contents of file
    // std::cout << l_ss.str() << std::endl;
    std::list<lexeme> l_lexemes;
    std::copy(std::istream_iterator<lexeme>(l_ss), std::istream_iterator<lexeme>(), std::back_inserter(l_lexemes));

    // assert(l_ss.eof());
    //     above line omitted due to trailing newline at end of file.
    assert(l_lexemes == std::list<lexeme>({
                            atom{"axiom"},
                            atom{"a0"},
                            list_open{},
                            atom{"if"},
                            atom{"y"},
                            atom{"x"},
                            list_close{},
                            eol{},
                            atom{"axiom"},
                            atom{"a1"},
                            atom{"x"},
                            eol{},
                            atom{"infer"},
                            atom{"i0"},
                            atom{"y"},
                            list_open{},
                            atom{"mp"},
                            list_open{},
                            atom{"theorem"},
                            atom{"a0"},
                            list_close{},
                            list_open{},
                            atom{"theorem"},
                            atom{"a1"},
                            list_close{},
                            list_close{},
                            eol{},
                        }));
}

void test_lex_file_example_5()
{
    using unilog::atom;
    using unilog::eol;
    using unilog::lexeme;
    using unilog::list_close;
    using unilog::list_open;
    using unilog::list_separator;
    using unilog::variable;

    std::stringstream l_ss;
    std::ifstream l_if("./src_test/example_unilog_files/lexer_example_5/jake.u");
    // std::cout << std::filesystem::current_path() << std::endl;
    // std::cout << "is_good: " << l_if.good() << std::endl;
    l_ss << l_if.rdbuf(); // read in contents of file
    // std::cout << l_ss.str() << std::endl;
    std::list<lexeme> l_lexemes;
    std::copy(std::istream_iterator<lexeme>(l_ss), std::istream_iterator<lexeme>(), std::back_inserter(l_lexemes));

    // assert(l_ss.eof());
    //     above line omitted due to trailing newline at end of file.
    assert(l_lexemes == std::list<lexeme>({
                            atom{"axiom"},
                            atom{"a0"},
                            list_open{},
                            atom{"awesome"},
                            variable{"_"},
                            list_close{},
                            eol{},

                            atom{"infer"},
                            atom{"i0"},
                            list_open{},
                            atom{"awesome"},
                            atom{"leon"},
                            list_close{},
                            list_open{},
                            atom{"theorem"},
                            atom{"a0"},
                            list_close{},
                            eol{},
                        }));
}

void test_lex_file_example_6()
{
    using unilog::atom;
    using unilog::eol;
    using unilog::lexeme;
    using unilog::list_close;
    using unilog::list_open;
    using unilog::list_separator;
    using unilog::variable;

    std::stringstream l_ss;
    std::ifstream l_if("./src_test/example_unilog_files/lexer_example_6/jake.u");
    // std::cout << std::filesystem::current_path() << std::endl;
    // std::cout << "is_good: " << l_if.good() << std::endl;
    l_ss << l_if.rdbuf(); // read in contents of file
    // std::cout << l_ss.str() << std::endl;
    std::list<lexeme> l_lexemes;
    std::copy(std::istream_iterator<lexeme>(l_ss), std::istream_iterator<lexeme>(), std::back_inserter(l_lexemes));

    assert(l_lexemes == std::list<lexeme>({
                            eol{},
                            eol{},
                            eol{},
                            atom{"guide"},
                            atom{"g0"},
                            list_open{},
                            list_close{},
                            list_open{},
                            atom{"theorem"},
                            variable{"_"},
                            list_close{},
                            atom{"asdasd"},
                            eol{},
                        }));
}

void test_lexer_main()
{
    constexpr bool ENABLE_DEBUG_LOGS = true;

    // equivalence tests
    TEST(test_lexer_escape);
    TEST(test_lexer_eol_equivalence);
    TEST(test_lexer_list_separator_equivalence);
    TEST(test_lexer_list_open_equivalence);
    TEST(test_lexer_list_close_equivalence);
    TEST(test_lexer_variable_equivalence);
    TEST(test_lexer_atom_equivalence);

    // extractor tests
    TEST(test_lexer_extract_lexeme);

    // larger tests, lexing files
    TEST(test_lex_file_example_0);
    TEST(test_lex_file_example_1);
    TEST(test_lex_file_example_2);
    TEST(test_lex_file_example_3);
    TEST(test_lex_file_example_4);
    TEST(test_lex_file_example_5);
    TEST(test_lex_file_example_6);
}
