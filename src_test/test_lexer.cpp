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

void test_lexer_quoted_atom_equivalence()
{
    using unilog::quoted_atom;

    data_points<std::pair<quoted_atom, quoted_atom>, bool> l_desired =
        {
            {
                {
                    quoted_atom{
                        "abc",
                    },
                    quoted_atom{
                        "abc",
                    },
                },
                true,
            },
            {
                {
                    quoted_atom{
                        "abc",
                    },
                    quoted_atom{
                        "abc1",
                    },
                },
                false,
            },
            {
                {
                    quoted_atom{
                        "",
                    },
                    quoted_atom{
                        "",
                    },
                },
                true,
            },
            {
                {
                    quoted_atom{
                        "abc",
                    },
                    quoted_atom{
                        "",
                    },
                },
                false,
            },
            {
                {
                    quoted_atom{
                        "",
                    },
                    quoted_atom{
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

void test_lexer_unquoted_atom_equivalence()
{
    using unilog::unquoted_atom;

    data_points<std::pair<unquoted_atom, unquoted_atom>, bool> l_desired =
        {
            {
                {
                    unquoted_atom{
                        "abc",
                    },
                    unquoted_atom{
                        "abc",
                    },
                },
                true,
            },
            {
                {
                    unquoted_atom{
                        "abc",
                    },
                    unquoted_atom{
                        "abc1",
                    },
                },
                false,
            },
            {
                {
                    unquoted_atom{
                        "",
                    },
                    unquoted_atom{
                        "",
                    },
                },
                true,
            },
            {
                {
                    unquoted_atom{
                        "abc",
                    },
                    unquoted_atom{
                        "",
                    },
                },
                false,
            },
            {
                {
                    unquoted_atom{
                        "",
                    },
                    unquoted_atom{
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

void test_lexer_extract_eol()
{
    constexpr bool ENABLE_DEBUG_LOGS = true;

    using unilog::eol;

    data_points<std::string, eol> l_desired =
        {
            {
                ";",
                eol{},
            },
            {
                "; []",
                eol{},
            },
            {
                "; [z]",
                eol{},
            },
            {
                "; [a b c]",
                eol{},
            },
            {
                ";[ \'quote\' ]",
                eol{},
            },
        };

    for (const auto &[l_key, l_value] : l_desired)
    {
        std::stringstream l_ss(l_key);

        eol l_eol;
        l_ss >> l_eol;

        // make sure the extraction was successful
        assert(!l_ss.fail());

        assert(l_eol == l_value);

        // make sure it did not extract more than it needs to
        l_ss.unget();
        assert(l_ss.peek() == ';');

        LOG("success, case: extracted eol: " << l_key << std::endl);
    }

    std::vector<std::string> l_expect_failure_inputs =
        {
            "",
            "!command",
            "!",
            ":",
            ":alg",
            ": ",
            "]",
            "Variable",
            "_Variable",
            "\'quoted\'",
            "unquoted",
            "@unquoted",
            "|",
        };

    for (const auto &l_input : l_expect_failure_inputs)
    {
        std::stringstream l_ss(l_input);

        eol l_eol;
        l_ss >> l_eol;

        // make sure the extraction was unsuccessful
        assert(l_ss.fail());

        LOG("success, case: expected failure extracting eol: " << l_input << std::endl);
    }
}

void test_lexer_extract_list_separator()
{
    constexpr bool ENABLE_DEBUG_LOGS = true;

    using unilog::list_separator;

    data_points<std::string, list_separator> l_desired =
        {
            {
                "|",
                list_separator{},
            },
            {
                "| []",
                list_separator{},
            },
            {
                "| [z]",
                list_separator{},
            },
            {
                "| [a b c]",
                list_separator{},
            },
            {
                "|[ \'quote\' ]",
                list_separator{},
            },
        };

    for (const auto &[l_key, l_value] : l_desired)
    {
        std::stringstream l_ss(l_key);

        list_separator l_list_separator;
        l_ss >> l_list_separator;

        // make sure the extraction was successful
        assert(!l_ss.fail());

        assert(l_list_separator == l_value);

        // make sure it did not extract more than it needs to
        l_ss.unget();
        assert(l_ss.peek() == '|');

        LOG("success, case: extracted list_separator: " << l_key << std::endl);
    }

    std::vector<std::string> l_expect_failure_inputs =
        {
            "",
            "!command",
            "!",
            ":",
            ":alg",
            ": ",
            "]",
            "Variable",
            "_Variable",
            "\'quoted\'",
            "unquoted",
            "@unquoted",
        };

    for (const auto &l_input : l_expect_failure_inputs)
    {
        std::stringstream l_ss(l_input);

        list_separator l_list_separator;
        l_ss >> l_list_separator;

        // make sure the extraction was unsuccessful
        assert(l_ss.fail());

        LOG("success, case: expected failure extracting list_separator: " << l_input << std::endl);
    }
}

void test_lexer_extract_list_open()
{
    constexpr bool ENABLE_DEBUG_LOGS = true;

    using unilog::list_open;

    data_points<std::string, list_open> l_desired =
        {
            {
                "[",
                list_open{},
            },
            {
                "[ ",
                list_open{},
            },
            {
                "[list]",
                list_open{},
            },
            {
                "[ list ]",
                list_open{},
            },
            {
                "[ \'quote\' ]",
                list_open{},
            },
        };

    for (const auto &[l_key, l_value] : l_desired)
    {
        std::stringstream l_ss(l_key);

        list_open l_list_open;
        l_ss >> l_list_open;

        // make sure the extraction was successful
        assert(!l_ss.fail());

        assert(l_list_open == l_value);

        // make sure it did not extract more than it needs to
        l_ss.unget();
        assert(l_ss.peek() == '[');

        LOG("success, case: extracted list_open: " << l_key << std::endl);
    }

    std::vector<std::string> l_expect_failure_inputs =
        {
            "",
            "!command",
            ":",
            ":alg",
            ": ",
            "|",
            "|[]",
            "| [a]",
            "!",
            "]",
            "Variable",
            "_Variable",
            "\'quoted\'",
            "unquoted",
            "@unquoted",
        };

    for (const auto &l_input : l_expect_failure_inputs)
    {
        std::stringstream l_ss(l_input);

        list_open l_list_open;
        l_ss >> l_list_open;

        // make sure the extraction was unsuccessful
        assert(l_ss.fail());

        LOG("success, case: expected failure extracting list_open: " << l_input << std::endl);
    }
}

void test_lexer_extract_list_close()
{
    constexpr bool ENABLE_DEBUG_LOGS = true;

    using unilog::list_close;

    data_points<std::string, list_close> l_desired =
        {
            {
                "]",
                list_close{},
            },
            {
                "] ",
                list_close{},
            },
            {
                "] list]",
                list_close{},
            },
            {
                "] list ]",
                list_close{},
            },
            {
                "] \'quote\' ]",
                list_close{},
            },
        };

    for (const auto &[l_key, l_value] : l_desired)
    {
        std::stringstream l_ss(l_key);

        list_close l_list_close;
        l_ss >> l_list_close;

        // make sure the extraction was successful
        assert(!l_ss.fail());

        assert(l_list_close == l_value);

        // make sure it did not extract more than it needs to
        l_ss.unget();
        assert(l_ss.peek() == ']');

        LOG("success, case: extracted list_close: " << l_key << std::endl);
    }

    std::vector<std::string> l_expect_failure_inputs =
        {
            "",
            "!command",
            ":",
            ":alg",
            ": ",
            "|",
            "|[]",
            "| [a]",
            "!",
            "[",
            "[]",
            "Variable",
            "_Variable",
            "\'quoted\'",
            "unquoted",
            "@unquoted",
        };

    for (const auto &l_input : l_expect_failure_inputs)
    {
        std::stringstream l_ss(l_input);

        list_close l_list_close;
        l_ss >> l_list_close;

        // make sure the extraction was unsuccessful
        assert(l_ss.fail());

        LOG("success, case: expected failure extracting list_close: " << l_input << std::endl);
    }
}

void test_lexer_extract_variable()
{
    constexpr bool ENABLE_DEBUG_LOGS = true;

    using unilog::variable;

    data_points<std::string, variable> l_desired =
        {
            {
                "_",
                variable{
                    "_",
                },
            },
            {
                "_ ",
                variable{
                    "_",
                },
            },
            {
                "_@",
                variable{
                    "_",
                },
            },
            {
                "_\'\'",
                variable{
                    "_",
                },
            },
            {
                "Variable!command",
                variable{
                    "Variable",
                },
            },
            {
                "Variable#sdfgsdfg",
                variable{
                    "Variable",
                },
            },
            {
                "_test",
                variable{
                    "_test",
                },
            },
            {
                "_test ",
                variable{
                    "_test",
                },
            },
            {
                "Test",
                variable{
                    "Test",
                },
            },
            {
                "Test ",
                variable{
                    "Test",
                },
            },
            {
                "Var123",
                variable{
                    "Var123",
                },
            },
            {
                "Var123_456k",
                variable{
                    "Var123_456k",
                },
            },
        };

    for (const auto &[l_key, l_value] : l_desired)
    {
        std::stringstream l_ss(l_key);

        variable l_variable;
        l_ss >> l_variable;

        // make sure the extraction was successful
        assert(!l_ss.fail());

        assert(l_variable == l_value);

        // make sure it did not extract more than it needs to
        l_ss.unget();
        assert(l_variable.m_identifier.size() == 0 || l_ss.peek() == l_variable.m_identifier.back());

        LOG("success, case: extracted variable: " << l_key << std::endl);
    }

    std::vector<std::string> l_expect_failure_inputs =
        {
            "",
            "!command",
            ":",
            ":alg",
            ": ",
            "|",
            "|[]",
            "| [a]",
            "!",
            "[",
            "]",
            "[]",
            "\'quoted\'",
            "unquoted",
            "@unquoted",
        };

    for (const auto &l_input : l_expect_failure_inputs)
    {
        std::stringstream l_ss(l_input);

        variable l_variable;
        l_ss >> l_variable;

        // make sure the extraction was unsuccessful
        assert(l_ss.fail());

        LOG("success, case: expected failure extracting variable: " << l_input << std::endl);
    }
}

void test_lexer_extract_quoted_atom()
{
    constexpr bool ENABLE_DEBUG_LOGS = true;

    using unilog::quoted_atom;

    data_points<std::string, quoted_atom> l_desired =
        {
            {
                "\'\'",
                quoted_atom{
                    "",
                },
            },
            {
                "\'[\'",
                quoted_atom{
                    "[",
                },
            },
            {
                "\']\'",
                quoted_atom{
                    "]",
                },
            },
            {
                "\'!\'",
                quoted_atom{
                    "!",
                },
            },
            {
                "\'a A \t\n b 1 3 zz\'",
                quoted_atom{
                    "a A \t\n b 1 3 zz",
                },
            },
            {
                "\'\\n\'",
                quoted_atom{
                    "\n",
                },
            },
            {
                "\'\\t\'",
                quoted_atom{
                    "\t",
                },
            },
            {
                "\'\\r\'",
                quoted_atom{
                    "\r",
                },
            },
            {
                "\'\\x88\'",
                quoted_atom{
                    "\x88",
                },
            },
            {
                "\'\\x88\'!command",
                quoted_atom{
                    "\x88",
                },
            },
            {
                "\'\\x88\'[list]",
                quoted_atom{
                    "\x88",
                },
            },
            {
                "\'\\x88\'\'quote2\'",
                quoted_atom{
                    "\x88",
                },
            },
            {
                "\'\\x88\'\"quote2\"",
                quoted_atom{
                    "\x88",
                },
            },

            {
                "\"\"",
                quoted_atom{
                    "",
                },
            },
            {
                "\"[\"",
                quoted_atom{
                    "[",
                },
            },
            {
                "\"]\"",
                quoted_atom{
                    "]",
                },
            },
            {
                "\"!\"",
                quoted_atom{
                    "!",
                },
            },
            {
                "\"a A \t\n b 1 3 zz\"",
                quoted_atom{
                    "a A \t\n b 1 3 zz",
                },
            },
            {
                "\"\\n\"",
                quoted_atom{
                    "\n",
                },
            },
            {
                "\"\\t\"",
                quoted_atom{
                    "\t",
                },
            },
            {
                "\"\\r\"",
                quoted_atom{
                    "\r",
                },
            },
            {
                "\"\\x88\"",
                quoted_atom{
                    "\x88",
                },
            },
            {
                "\"\\x88\"!command",
                quoted_atom{
                    "\x88",
                },
            },
            {
                "\"\\x88\"[list]",
                quoted_atom{
                    "\x88",
                },
            },
            {
                "\"\\x88\"\'quote2\'",
                quoted_atom{
                    "\x88",
                },
            },
            {
                "\"\\x88\"\"quote2\"",
                quoted_atom{
                    "\x88",
                },
            },

        };

    for (const auto &[l_key, l_value] : l_desired)
    {
        std::stringstream l_ss(l_key);

        quoted_atom l_quoted_atom;
        l_ss >> l_quoted_atom;

        // make sure the extraction was successful
        assert(!l_ss.fail());

        assert(l_quoted_atom == l_value);

        // ensure the trailing quote was consumed
        l_ss.unget();
        assert(l_ss.peek() == '\'' || l_ss.peek() == '\"');

        LOG("success, case: extracted quoted_atom: " << l_key << std::endl);
    }

    std::vector<std::string> l_expect_failure_inputs =
        {
            "",
            "!command",
            "!",
            ":",
            ":alg",
            ": ",
            "|",
            "|[]",
            "| [a]",
            "[",
            "]",
            "[]",
            "_",
            "_ ",
            "_Variable",
            "Variable",
            "#",
            "$",
            "\'halfquote",
            "\"halfquote",
            "unquoted",
            "@unquoted",
        };

    for (const auto &l_input : l_expect_failure_inputs)
    {
        std::stringstream l_ss(l_input);

        quoted_atom l_quoted_atom;
        l_ss >> l_quoted_atom;

        // make sure the extraction was unsuccessful
        assert(l_ss.fail());

        LOG("success, case: expected failure extracting quoted_atom: " << l_input << std::endl);
    }
}

void test_lexer_extract_unquoted_atom()
{
    constexpr bool ENABLE_DEBUG_LOGS = true;

    using unilog::unquoted_atom;

    data_points<std::string, unquoted_atom> l_desired =
        {
            {
                "x",
                unquoted_atom{
                    "x",
                },
            },
            {
                "a ",
                unquoted_atom{
                    "a",
                },
            },
            {
                "a1 ",
                unquoted_atom{
                    "a1",
                },
            },
            {
                "a1@",
                unquoted_atom{
                    "a1@",
                },
            },
            {
                "a1.",
                unquoted_atom{
                    "a1.",
                },
            },
            {
                "1.24",
                unquoted_atom{
                    "1.24",
                },
            },
            {
                "atom!",
                unquoted_atom{
                    "atom!",
                },
            },
            {
                "at\\",
                unquoted_atom{
                    "at\\",
                },
            },
            {
                "at@#$%^&*()-_=+",
                unquoted_atom{
                    "at@#$%^&*()-_=+",
                },
            },
            {
                "at\'quote\'",
                unquoted_atom{
                    "at",
                },
            },
            {
                "at\"quote\"",
                unquoted_atom{
                    "at",
                },
            },
            {
                "aTOM\"quote\"",
                unquoted_atom{
                    "aTOM",
                },
            },
            {
                "a123\"quote\"",
                unquoted_atom{
                    "a123",
                },
            },
            {
                "a\\\\\"quote\"",
                unquoted_atom{
                    "a\\\\",
                },
            },
            {
                "a/\"quote\"",
                unquoted_atom{
                    "a/",
                },
            },
            {
                "123",
                unquoted_atom{
                    "123",
                },
            },
            {
                "@#$%^&*()",
                unquoted_atom{
                    "@#$%^&*()",
                },
            },
        };

    for (const auto &[l_key, l_value] : l_desired)
    {
        std::stringstream l_ss(l_key);

        unquoted_atom l_unquoted_atom;
        l_ss >> l_unquoted_atom;

        // make sure the extraction was successful
        assert(!l_ss.fail());

        assert(l_unquoted_atom == l_value);

        // make sure it did not extract more than it needs to
        l_ss.unget();
        assert(l_unquoted_atom.m_text.size() == 0 || l_ss.peek() == l_unquoted_atom.m_text.back());

        LOG("success, case: extracted unquoted_atom: " << l_key << std::endl);
    }

    std::vector<std::string> l_expect_failure_inputs =
        {
            "",
            ";",
            ";a",
            "|",
            "|[]",
            "| [a]",
            "[",
            "]",
            "[]",
            "_",
            "_ ",
            "Variable",
            "Var123",
            "X",
            "\'quoted\'",
            "\"quoted\"",
        };

    for (const auto &l_input : l_expect_failure_inputs)
    {
        std::stringstream l_ss(l_input);

        unquoted_atom l_unquoted_atom;
        l_ss >> l_unquoted_atom;

        // make sure the extraction was unsuccessful
        assert(l_ss.fail());

        LOG("success, case: expected failure extracting unquoted_atom: " << l_input << std::endl);
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
    using unilog::quoted_atom;
    using unilog::unquoted_atom;
    using unilog::variable;

    // Cases where content should be left alone:
    data_points<std::string, std::vector<lexeme>> l_desired_map = {
        {
            // Single unquoted_atom lexeme
            "!test",
            std::vector<lexeme>{
                unquoted_atom{
                    .m_text = "!test",
                },
            },
        },
        {
            // Multiple unquoted_atom lexeme
            "!test1 !test2",
            std::vector<lexeme>{
                unquoted_atom{
                    .m_text = "!test1",
                },
                unquoted_atom{
                    .m_text = "!test2",
                },
            },
        },
        {
            ":",
            std::vector<lexeme>{
                unquoted_atom{
                    .m_text = ":",
                },
            },
        },
        {
            ":m2",
            std::vector<lexeme>{
                unquoted_atom{
                    ":m2",
                },
            },
        },
        {
            "m1:m2",
            std::vector<lexeme>{
                unquoted_atom{
                    "m1:m2",
                },
            },
        },
        {
            "m1:X",
            std::vector<lexeme>{
                unquoted_atom{
                    "m1:X",
                },
            },
        },
        {
            "m1::[a b c]",
            std::vector<lexeme>{
                unquoted_atom{
                    "m1::",
                },
                list_open{},
                unquoted_atom{
                    "a",
                },
                unquoted_atom{
                    "b",
                },
                unquoted_atom{
                    "c",
                },
                list_close{},
            },
        },
        {
            "!axiom [m1::a m1::b m1::c]",
            std::vector<lexeme>{
                unquoted_atom{
                    "!axiom",
                },
                list_open{},
                unquoted_atom{
                    "m1::a",
                },
                unquoted_atom{
                    "m1::b",
                },
                unquoted_atom{
                    "m1::c",
                },
                list_close{},
            },
        },
        {
            "a|",
            std::vector<lexeme>{
                unquoted_atom{
                    "a",
                },
                list_separator{},
            },
        },
        {
            "a|[]",
            std::vector<lexeme>{
                unquoted_atom{
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
                unquoted_atom{
                    "a",
                },
                list_separator{},
                list_open{},
                list_close{},
                unquoted_atom{
                    "b",
                },
                list_separator{},
                list_open{},
                list_close{},
                unquoted_atom{
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
                unquoted_atom{
                    .m_text = "test",
                },
            },
        },
        {
            // Command then unquoted atom
            "!test1 test2",
            std::vector<lexeme>{
                unquoted_atom{
                    .m_text = "!test1",
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
                unquoted_atom{
                    .m_text = "!test2",
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
                    .m_text = "test!@.$^&*()",
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
                unquoted_atom{
                    .m_text = "!axiom",
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
                unquoted_atom{
                    .m_text = "!axiom",
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
                unquoted_atom{
                    .m_text = "!test1",
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
                unquoted_atom{
                    .m_text = "!test1",
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
                unquoted_atom{
                    .m_text = "!Test",
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
                unquoted_atom{
                    "!tes@t",
                },
            },
        },
        {
            "!test1\\!test2",
            std::vector<lexeme>{
                unquoted_atom{
                    "!test1\\!test2",
                },
            },
        },
        {
            "!test1!test2",
            std::vector<lexeme>{
                unquoted_atom{
                    "!test1!test2",
                },
            },
        },
        {
            "test;",
            std::vector<lexeme>{
                unquoted_atom{"test"},
                eol{},
            },
        },
        {
            "test1;test2",
            std::vector<lexeme>{
                unquoted_atom{"test1"},
                eol{},
                unquoted_atom{"test2"},
            },
        },
        {
            "[test1 test2];",
            std::vector<lexeme>{
                list_open{},
                unquoted_atom{"test1"},
                unquoted_atom{"test2"},
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
                unquoted_atom{"test1"},
                eol{},
                quoted_atom{"test2"},
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
    using unilog::quoted_atom;
    using unilog::unquoted_atom;
    using unilog::variable;

    std::stringstream l_ss;
    std::ifstream l_if("./src_test/example_unilog_files/lexer_example_0/main.ul");
    // std::cout << std::filesystem::current_path() << std::endl;
    // std::cout << "is_good: " << l_if.good() << std::endl;
    l_ss << l_if.rdbuf(); // read in contents of file
    std::cout << l_ss.str() << std::endl;

    std::list<lexeme> l_lexemes;
    std::copy(std::istream_iterator<lexeme>(l_ss), std::istream_iterator<lexeme>(), std::back_inserter(l_lexemes));

    assert(l_ss.eof()); // assert successful extraction
    assert(l_lexemes == std::list<lexeme>({
                            unquoted_atom{"axiom"},
                            unquoted_atom{"a0"},
                            list_open{},
                            unquoted_atom{"if"},
                            unquoted_atom{"y"},
                            unquoted_atom{"x"},
                            list_close{},
                            unquoted_atom{"axiom"},
                            unquoted_atom{"a1"},
                            unquoted_atom{"x"},
                        }));
}

void test_lex_file_example_1()
{
    using unilog::atom;
    using unilog::lexeme;
    using unilog::list_close;
    using unilog::list_open;
    using unilog::list_separator;
    using unilog::quoted_atom;
    using unilog::unquoted_atom;
    using unilog::variable;

    std::stringstream l_ss;
    std::ifstream l_if("./src_test/example_unilog_files/lexer_example_1/main.ul");
    // std::cout << std::filesystem::current_path() << std::endl;
    // std::cout << "is_good: " << l_if.good() << std::endl;
    l_ss << l_if.rdbuf(); // read in contents of file
    std::cout << l_ss.str() << std::endl;

    std::list<lexeme> l_lexemes;
    std::copy(std::istream_iterator<lexeme>(l_ss), std::istream_iterator<lexeme>(), std::back_inserter(l_lexemes));

    assert(l_ss.eof()); // assert successful extraction
    assert(l_lexemes == std::list<lexeme>({
                            unquoted_atom{"axiom"},
                            quoted_atom{"a0"},
                            unquoted_atom{"test"},

                            unquoted_atom{"axiom"},
                            list_open{},
                            unquoted_atom{"a1"},
                            list_close{},
                            quoted_atom{"x"},

                            unquoted_atom{"axiom"},
                            quoted_atom{"a2"},
                            list_open{},
                            unquoted_atom{"x"},
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
    using unilog::quoted_atom;
    using unilog::unquoted_atom;
    using unilog::variable;

    std::stringstream l_ss;
    std::ifstream l_if("./src_test/example_unilog_files/lexer_example_2/main.ul");
    // std::cout << std::filesystem::current_path() << std::endl;
    // std::cout << "is_good: " << l_if.good() << std::endl;
    l_ss << l_if.rdbuf(); // read in contents of file
    std::cout << l_ss.str() << std::endl;

    std::list<lexeme> l_lexemes;
    std::copy(std::istream_iterator<lexeme>(l_ss), std::istream_iterator<lexeme>(), std::back_inserter(l_lexemes));

    assert(l_ss.eof()); // assert successful extraction
    assert(l_lexemes == std::list<lexeme>({
                            unquoted_atom{"infer"},
                            unquoted_atom{"i0"},
                            list_open{},
                            unquoted_atom{"if"},
                            variable{"Y"},
                            list_open{},
                            unquoted_atom{"and"},
                            list_open{},
                            unquoted_atom{"if"},
                            variable{"Y"},
                            variable{"X"},
                            list_close{},
                            variable{"X"},
                            list_close{},
                            list_close{},

                            unquoted_atom{"axiom"},
                            unquoted_atom{"a0"},
                            list_open{},
                            unquoted_atom{"if"},
                            unquoted_atom{"b"},
                            unquoted_atom{"a"},
                            list_close{},

                            unquoted_atom{"axiom"},
                            unquoted_atom{"a1"},
                            unquoted_atom{"a"},
                        }));
}

void test_lex_file_example_3()
{
    using unilog::atom;
    using unilog::lexeme;
    using unilog::list_close;
    using unilog::list_open;
    using unilog::list_separator;
    using unilog::quoted_atom;
    using unilog::unquoted_atom;
    using unilog::variable;

    std::stringstream l_ss;
    std::ifstream l_if("./src_test/example_unilog_files/lexer_example_3/main.ul");
    // std::cout << std::filesystem::current_path() << std::endl;
    // std::cout << "is_good: " << l_if.good() << std::endl;
    l_ss << l_if.rdbuf(); // read in contents of file
    std::cout << l_ss.str() << std::endl;
    std::cout << "eof?: " << ((l_ss.peek() == std::istream::traits_type::eof()) ? '1' : '0') << std::endl;
    std::list<lexeme> l_lexemes;
    std::copy(std::istream_iterator<lexeme>(l_ss), std::istream_iterator<lexeme>(), std::back_inserter(l_lexemes));

    assert(l_ss.peek() == EOF); // assert successful extraction
    assert(l_lexemes == std::list<lexeme>({

                        }));
}

void test_lexer_main()
{
    constexpr bool ENABLE_DEBUG_LOGS = true;

    TEST(test_lexer_escape);
    TEST(test_lexer_eol_equivalence);
    TEST(test_lexer_list_separator_equivalence);
    TEST(test_lexer_list_open_equivalence);
    TEST(test_lexer_list_close_equivalence);
    TEST(test_lexer_variable_equivalence);
    TEST(test_lexer_quoted_atom_equivalence);
    TEST(test_lexer_unquoted_atom_equivalence);
    TEST(test_lexer_extract_eol);
    TEST(test_lexer_extract_list_separator);
    TEST(test_lexer_extract_list_open);
    TEST(test_lexer_extract_list_close);
    TEST(test_lexer_extract_variable);
    TEST(test_lexer_extract_quoted_atom);
    TEST(test_lexer_extract_unquoted_atom);
    TEST(test_lexer_extract_lexeme);
    TEST(test_lex_file_example_0);
    TEST(test_lex_file_example_1);
    TEST(test_lex_file_example_2);
    TEST(test_lex_file_example_3);

    return;
}
