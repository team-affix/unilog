#include <iostream>
#include <SWI-Prolog.h>

#define LOG(x)             \
    if (ENABLE_DEBUG_LOGS) \
        std::cout << x;

#define TEST(void_fn) \
    void_fn();        \
    LOG("TEST COMPLETED: " << #void_fn << std::endl);

extern void test_lexer_main();
extern void test_parser_main();

void unit_test_main()
{
    constexpr bool ENABLE_DEBUG_LOGS = true;

    TEST(test_lexer_main);
    TEST(test_parser_main);
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
