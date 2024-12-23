#include <string>
#include <regex>
#include <cctype>
#include <stdexcept>

#include "lexer.hpp"
#include "err_msg.hpp"

std::istream &escape(std::istream &a_istream, char &a_char)
{
    char l_char;
    a_istream.get(l_char);

    switch (l_char)
    {
    case '0':
    {
        a_char = '\0';
    }
    break;
    case 'a':
    {
        a_char = '\a';
    }
    break;
    case 'b':
    {
        a_char = '\b';
    }
    break;
    case 't':
    {
        a_char = '\t';
    }
    break;
    case 'n':
    {
        a_char = '\n';
    }
    break;
    case 'v':
    {
        a_char = '\v';
    }
    break;
    case 'f':
    {
        a_char = '\f';
    }
    break;
    case 'r':
    {
        a_char = '\r';
    }
    break;
    case 'x':
    {
        char l_upper_hex_digit;
        char l_lower_hex_digit;

        if (!a_istream.get(l_upper_hex_digit))
            break;
        if (!a_istream.get(l_lower_hex_digit))
            break;

        if (std::isxdigit(l_upper_hex_digit) == 0 || std::isxdigit(l_lower_hex_digit) == 0)
        {
            a_istream.setstate(std::ios::failbit);
            break;
        }

        int l_hex_value;

        std::stringstream l_ss;

        l_ss << std::hex << l_upper_hex_digit << l_lower_hex_digit;

        l_ss >> l_hex_value;

        a_char = static_cast<char>(l_hex_value);
    }
    break;
    default:
    {
        a_char = l_char;
    }
    break;
    }

    return a_istream;
}

static std::istream &consume_line(std::istream &a_istream)
{
    int l_char;

    // Extract character (read all chars up to and including \n. If we peek EOF, return before consumption)
    while (
        a_istream.good() &&
        (l_char = a_istream.peek()) &&
        (l_char != std::ios::traits_type::eof()) &&
        (a_istream.get() && l_char != '\n'))
        ;

    return a_istream;
}

static std::istream &consume_whitespace(std::istream &a_istream)
{
    int l_char;

    // Extract character (read until first non-whitespace)
    while (
        a_istream.good() && // important to check a_istream.good() to prevent double-peeking an EOF, which will cause failbit.
        (l_char = a_istream.peek()) &&
        ((std::isspace(l_char) != 0 && a_istream.get()) ||
         (l_char == '#' && consume_line(a_istream))))
        ;

    return a_istream;
}

static std::istream &extract_unquoted_text(std::istream &a_istream, std::string &a_text)
{
    ////////////////////////////////////
    /////////// TEXT SECTION ///////////
    ////////////////////////////////////

    a_text.clear();

    char l_char;

    // unquoted lexemes may only contain alphanumeric chars and underscores.
    while (
        l_char = a_istream.peek(),
        // conditions for consumption
        (isalnum(l_char) || l_char == '_') &&
            // Consume char now
            a_istream.get(l_char))
    {
        a_text.push_back(l_char);
    }

    return a_istream;
}

static std::istream &extract_quoted_text(std::istream &a_istream, std::string &a_text)
{
    ////////////////////////////////////
    /////////// TEXT SECTION ///////////
    ////////////////////////////////////

    // save the type of quotation. then we can match for closing quote.
    int l_quote_char = a_istream.get();

    a_text.clear();

    char l_char;

    // The input was a quote character. Thus we should scan until the closing quote
    //     to produce a valid lexeme.
    while (
        // Consume char now
        a_istream.get(l_char))
    {
        // no multiline string literals
        if (l_char == '\n')
            throw std::runtime_error(ERR_MSG_CLOSING_QUOTE);

        if (l_char == l_quote_char)
            break;

        if (l_char == '\\')
            escape(a_istream, l_char);

        a_text.push_back(l_char);
    }

    // if we fail to extract character, then throw exception
    if (a_istream.fail())
        throw std::runtime_error(ERR_MSG_CLOSING_QUOTE);

    return a_istream;
}

namespace unilog
{

    bool operator==(const eol &a_lhs, const eol &a_rhs)
    {
        return true;
    }

    bool operator==(const list_separator &a_lhs, const list_separator &a_rhs)
    {
        return true;
    }

    bool operator==(const list_open &a_lhs, const list_open &a_rhs)
    {
        return true;
    }

    bool operator==(const list_close &a_lhs, const list_close &a_rhs)
    {
        return true;
    }

    bool operator==(const variable &a_lhs, const variable &a_rhs)
    {
        return a_lhs.m_identifier == a_rhs.m_identifier;
    }

    bool operator==(const atom &a_lhs, const atom &a_rhs)
    {
        return a_lhs.m_text == a_rhs.m_text;
    }

    std::istream &operator>>(std::istream &a_istream, lexeme &a_lexeme)
    {
        // consume all leading whitespace
        consume_whitespace(a_istream);

        // get the char which indicates type of lexeme
        int l_indicator = a_istream.peek();

        if (!a_istream.good())
            return a_istream;

        if (l_indicator == ';')
        {
            a_istream.get(); // extract the character
            a_lexeme = eol{};
        }
        else if (l_indicator == '|')
        {
            a_istream.get(); // extract the character
            a_lexeme = list_separator{};
        }
        else if (l_indicator == '[')
        {
            a_istream.get(); // extract the character
            a_lexeme = list_open{};
        }
        else if (l_indicator == ']')
        {
            a_istream.get(); // extract the character
            a_lexeme = list_close{};
        }
        else if (isupper(l_indicator) || l_indicator == '_')
        {
            variable l_result;
            extract_unquoted_text(a_istream, l_result.m_identifier);
            a_lexeme = l_result;
        }
        else if (l_indicator == '\'' || l_indicator == '\"')
        {
            atom l_result;
            extract_quoted_text(a_istream, l_result.m_text);
            a_lexeme = l_result;
        }
        else if (isalpha(l_indicator)) // only lower-case letters
        {
            atom l_result;
            extract_unquoted_text(a_istream, l_result.m_text);
            a_lexeme = l_result;
        }
        else
        {
            throw std::runtime_error(ERR_MSG_INVALID_LEXEME);
        }

        return a_istream;
    }

}

#ifdef UNIT_TEST

#include <fstream>
#include <iterator>
#include <vector>
#include "test_utils.hpp"

static void test_lexer_escape()
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

static void test_consume_line()
{
    data_points<std::string, std::streampos> l_data_points =
        {
            {"\nakdsfhjghdjfgj", 1},
            {"a\nakdsfhjghdjfgj", 2},
            {"abcdefg\nakdsfhjghdjfgj", 8},
            {"   \nakdsfhjghdjfgj", 4},
            {"[] {} ( ? // sdfgfjgfdkjgl cxcsdc # fdsfdsfkf \nakdsfhjghdjfgj", 47},
            {"[] {} ( ? // sdfgfjgfdkj\r\t cxcsdc # fdsfdsfkf \nakdsfhjghdjfgj", 47},
            {"abc", -1},        // in case of EOF, treat same as newline.
            {"\t\tabdsd ", -1}, // in case of EOF, treat same as newline.
        };

    for (const auto &[l_key, l_value] : l_data_points)
    {
        std::stringstream l_ss(l_key);
        assert(consume_line(l_ss)); // eofbit = 2
        // std::cout << l_ss.rdstate() << std::endl;
        assert(l_ss.tellg() == l_value); // make sure we extracted the correct # of chars
    }
}

static void test_consume_whitespace()
{
    data_points<std::string, std::streampos> l_data_points =
        {
            {"    abc", 4},
            {"   \nabc", 4},
            {"  \t\nabc", 4},
            {" \r\t\nabc", 4},
            {"\r  \n\r\t\nabc", 7},
            {"\r  \n\r\t\na", 7},
            {"\r  \n\r\t\n1", 7},
            {"\r  \n\r\t\n1 \t", 7},
            {"\r  \n\r\t\n0- \t", 7},
            {"\r  \n\r\t\n/ \t", 7},
            {"\r  \n\r\t\n? \t", 7},
            {"a", 0},
            {"\r  \n#\r\t\n? \t", 8},       // # marks the start of a comment, thus marking rest of line as whitespace
            {"  \t \r \n \t #abcd\ne", 16}, // # marks the start of a comment, thus marking rest of line as whitespace
            {"\t# abcdefg", -1},
        };

    for (const auto &[l_key, l_value] : l_data_points)
    {
        std::stringstream l_ss(l_key);
        assert(consume_whitespace(l_ss));
        assert(l_ss.tellg() == l_value); // make sure we extracted the correct # of chars
    }
}

static void test_extract_unquoted_text()
{
    data_points<std::string, std::string> l_data_points =
        {
            {"/abc", ""},
            {"a/abc", "a"},
            {" abc", ""},                                                  // consume_whitespace() will not be called.
            {"abcdefghijklmnopqrstuvwxyz", "abcdefghijklmnopqrstuvwxyz"},  // all lowers
            {"abcdefghijklmnopqrstuvwxyz ", "abcdefghijklmnopqrstuvwxyz"}, // trailing white space
            {"AbCdEfGhIjKlMnOpQrStUvWxYz", "AbCdEfGhIjKlMnOpQrStUvWxYz"},  // mixed upper/lower
            {"abc_ ", "abc_"},                                             // containing underscore
            {"123 ", "123"},                                               // containing numbers
            {"abc123 ", "abc123"},                                         // alphanumeric
            {"abc_1_23 ", "abc_1_23"},                                     // alphanumeric with underscores
            {"abC_D_23 ", "abC_D_23"},                                     // alphanumeric with underscores and uppers
            {"_abC_D_23 ", "_abC_D_23"},                                   // alphanumeric with underscores, leading underscore
            {"_abC_D_23\n", "_abC_D_23"},                                  // symbol terminating
            {"_abC_D_23\n\t", "_abC_D_23"},                                // symbol terminating
            {"_abC_D_23/", "_abC_D_23"},                                   // symbol terminating
            {"_abC_D_23\\", "_abC_D_23"},                                  // symbol terminating
            {"_abC_D_23|", "_abC_D_23"},                                   // symbol terminating
            {"_abC_D_23[", "_abC_D_23"},                                   // symbol terminating
            {"_abC_D_23\t", "_abC_D_23"},                                  // symbol terminating
            {"_abC_D_23\r", "_abC_D_23"},                                  // symbol terminating
            {"_abC_D_23 abc", "_abC_D_23"},                                // two unquoted texts separated by space
            {"\r_abC_D_23", ""},                                           // symbol terminating
            {"\n_abC_D_23", ""},                                           // symbol terminating
            {"\t_abC_D_23", ""},                                           // symbol terminating
        };

    for (const auto &[l_key, l_value] : l_data_points)
    {
        std::stringstream l_ss(l_key);
        std::string l_string;
        assert(extract_unquoted_text(l_ss, l_string));
        assert(l_string == l_value);
    }
}

static void test_extract_quoted_text()
{
    data_points<std::string, std::string> l_data_points =
        {
            {"\'/abc\'", "/abc"},
            {"\'a/abc\'", "a/abc"},
            {"\' abc\'", " abc"},
            {"\"abcdefghijklmnopqrstuvwxyz\"", "abcdefghijklmnopqrstuvwxyz"},
            {"\"abcdefghijklmnopqrstuvwxyz \"", "abcdefghijklmnopqrstuvwxyz "},
            {"\'AbCdEfGhIjKlMnOpQrStUvWxYz\'", "AbCdEfGhIjKlMnOpQrStUvWxYz"},
            {"\'abc_ \'", "abc_ "},
            {"\'123 \'", "123 "},
            {"\'abc123 \'", "abc123 "},
            {"\'abc_1_23 \'", "abc_1_23 "},
            {"\' abC_D_23\' ", " abC_D_23"},
            {"\"_abC_D_23\" ", "_abC_D_23"},
            {"\'_abC_D_23\'", "_abC_D_23"},
            {"\"_abC_D_23\t\"", "_abC_D_23\t"},
            {"\"_abC_D_2\"3/", "_abC_D_2"},
            {"\'_abC_D_23\\\\\'", "_abC_D_23\\"}, // extract_quote calls escape() when encountering a backslash.
            {"\'abc 123\'", "abc 123"},
            {"\"_abC_D_23[\"", "_abC_D_23["},
            {"\"\t_abC_D_23\t\"", "\t_abC_D_23\t"},
            {"\'_abC_D_23\r\'", "_abC_D_23\r"},
            {"\"_abC_D_23\" abc", "_abC_D_23"},
            {"\'\r_abC_D_23\'", "\r_abC_D_23"},
            {"\"_abC_D_23  \t \"", "_abC_D_23  \t "},
            {"\'\t_abC_D_23\'", "\t_abC_D_23"},
            {"\'\\n\'", "\n"},
            {"\'\\t\'", "\t"},
            {"\'\\tabc\'", "\tabc"},
            {"\'\\x08abc\'", "\x08"
                             "abc"},
            {"\'\\rabc\'", "\rabc"},
            {"\'\\r1\\n23\'", "\r1\n23"},

            {"\"\\n\"", "\n"},
            {"\"\\t\"", "\t"},
            {"\"\\tabc\"", "\tabc"},
            {"\"\\x08abc\"", "\x08"
                             "abc"},
            {"\"\\rabc\"", "\rabc"},
            {"\"\\r1\\n23\"", "\r1\n23"},
        };

    for (const auto &[l_key, l_value] : l_data_points)
    {
        std::stringstream l_ss(l_key);
        std::string l_string;
        assert(extract_quoted_text(l_ss, l_string));
        assert(l_string == l_value);
    }
}

static void test_lexer_eol_equivalence()
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

static void test_lexer_list_separator_equivalence()
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

static void test_lexer_list_open_equivalence()
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

static void test_lexer_list_close_equivalence()
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

static void test_lexer_variable_equivalence()
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

static void test_lexer_atom_equivalence()
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

static void test_lexer_extract_lexeme()
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
        {
            "; # comment\n"
            ";;# abc123\n"
            "[test1;# I AM A COMMENT\n"
            "\"test2\"];\n",
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
        {
            "abc#[]",
            std::vector<lexeme>{
                atom{"abc"},
            },
        },
        {
            "abc#[| dasdsad \t\r\n]",
            std::vector<lexeme>{
                atom{"abc"},
                list_close{},
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

    data_points<std::string, std::string> l_expect_throw_inputs =
        {
            {"\'hello, this is an unclosed quote", ERR_MSG_CLOSING_QUOTE},
            {"\"hello, this is an unclosed quote", ERR_MSG_CLOSING_QUOTE},
            {"!test1!test2", ERR_MSG_INVALID_LEXEME},
            {"!test1\\!test2", ERR_MSG_INVALID_LEXEME},

            {"!Test", ERR_MSG_INVALID_LEXEME},
            {"!test1[abc]", ERR_MSG_INVALID_LEXEME},
            {"<-", ERR_MSG_INVALID_LEXEME},
            {"+", ERR_MSG_INVALID_LEXEME},
            {"@", ERR_MSG_INVALID_LEXEME},
            {"1.24", ERR_MSG_INVALID_LEXEME},
            {"\\test", ERR_MSG_INVALID_LEXEME},

            // atoms that start with numbers
            {"1ab", ERR_MSG_INVALID_LEXEME},

            // special symbols
            {"!", ERR_MSG_INVALID_LEXEME},
            {"@", ERR_MSG_INVALID_LEXEME},
            {"$", ERR_MSG_INVALID_LEXEME},
            {"%", ERR_MSG_INVALID_LEXEME},
            {"^", ERR_MSG_INVALID_LEXEME},
            {"&", ERR_MSG_INVALID_LEXEME},
            {"*", ERR_MSG_INVALID_LEXEME},
            {"(", ERR_MSG_INVALID_LEXEME},
            {")", ERR_MSG_INVALID_LEXEME},
            {"-", ERR_MSG_INVALID_LEXEME},
            {"=", ERR_MSG_INVALID_LEXEME},

        };

    for (const auto &[l_input, l_err_msg] : l_expect_throw_inputs)
    {
        std::stringstream l_ss(l_input);

        lexeme l_lexeme;

        // Make sure the case throws an exception.
        try
        {
            l_ss >> l_lexeme;
            throw std::runtime_error("Failed test case: expected throw");
        }
        catch (const std::runtime_error &l_err)
        {
            assert(l_err.what() == l_err_msg);
        }

        LOG("success, expected throw, case: " << l_input << std::endl);
    }

    std::vector<std::string> l_expect_failbit_inputs =
        {
            "    ",
            "",

            "#Test",

            "#",
        };

    for (const auto &l_input : l_expect_failbit_inputs)
    {
        std::stringstream l_ss(l_input);

        lexeme l_lexeme;

        l_ss >> l_lexeme;

        // Make sure the stream state has its failbit set.
        assert(l_ss.fail());

        LOG("success, expected stream fail state, case: " << l_input << std::endl);
    }
}

static void test_lex_file_examples()
{
    constexpr bool ENABLE_DEBUG_LOGS = true;

    using unilog::atom;
    using unilog::eol;
    using unilog::lexeme;
    using unilog::list_close;
    using unilog::list_open;
    using unilog::list_separator;
    using unilog::variable;

    data_points<std::string, std::list<lexeme>> l_data_points =
        {
            {
                "./src/test_input_files/lexer_example_0/main.ul",
                {
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
                },
            },
            {
                "./src/test_input_files/lexer_example_1/main.ul",
                {
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
                },
            },
            {
                "./src/test_input_files/lexer_example_2/main.ul",
                {
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
                },
            },
            {
                "./src/test_input_files/lexer_example_3/main.ul",
                {
                    // empty, no lexemes
                },
            },
            {
                "./src/test_input_files/lexer_example_4/jake.u",
                {
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
                },
            },
            {
                "./src/test_input_files/lexer_example_5/jake.u",
                {
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
                },
            },
            {
                "./src/test_input_files/lexer_example_6/jake.u",
                {
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
                },
            },
        };

    for (const auto &[l_file_path, l_expected] : l_data_points)
    {
        std::ifstream l_if(l_file_path);

        std::list<lexeme> l_lexemes;
        std::copy(std::istream_iterator<lexeme>(l_if), std::istream_iterator<lexeme>(), std::back_inserter(l_lexemes));

        assert(l_lexemes == l_expected);

        LOG("success, lexed file: " << l_file_path << std::endl);
    }
}

void test_lexer_main()
{
    constexpr bool ENABLE_DEBUG_LOGS = true;

    // equivalence tests
    TEST(test_lexer_escape);
    TEST(test_consume_line);
    TEST(test_consume_whitespace);
    TEST(test_extract_unquoted_text);
    TEST(test_extract_quoted_text);
    TEST(test_lexer_eol_equivalence);
    TEST(test_lexer_list_separator_equivalence);
    TEST(test_lexer_list_open_equivalence);
    TEST(test_lexer_list_close_equivalence);
    TEST(test_lexer_variable_equivalence);
    TEST(test_lexer_atom_equivalence);

    // extractor tests
    TEST(test_lexer_extract_lexeme);

    // larger tests, lexing files
    TEST(test_lex_file_examples);
}

#endif
