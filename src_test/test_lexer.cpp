#include <iostream>

#include "../src_lib/lexer.hpp"

#define LOG(x)             \
    if (ENABLE_DEBUG_LOGS) \
        std::cout << x;

#define TEST(void_fn) \
    void_fn();        \
    LOG("TEST COMPLETED: " << #void_fn << std::endl);

int test_lexer_main()
{
    std::cout << "linked: " << unilog::fxn() << std::endl;

    return 0;
}
