#include <iostream>
#include <sstream>
#include <assert.h>
#include <map>

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
}

int test_lexer_main()
{
    constexpr bool ENABLE_DEBUG_LOGS = true;

    std::cout << "linked: " << unilog::fxn() << std::endl;

    TEST(test_lexer_escape);

    return 0;
}
