#include <iostream>
#include "../swipl/src/SWI-Prolog.h"

#define LOG(x)             \
    if (ENABLE_DEBUG_LOGS) \
        std::cout << x;

#define TEST(void_fn) \
    void_fn();        \
    LOG("TEST COMPLETED: " << #void_fn << std::endl);

extern int test_lexer_main();

int unit_test_main()
{
    constexpr bool ENABLE_DEBUG_LOGS = true;

    TEST(test_lexer_main);

    return 0;
}

int main(int argc, char **argv)
{
    const char *plav[] = {argv[0], "--quiet", "--nosignals"};

    /* initialise Prolog */
    if (!PL_initialise(3, const_cast<char **>(plav)))
        PL_halt(1);

    unit_test_main();

    PL_halt(0); // Properly halt the Prolog engine
    return 0;
}
