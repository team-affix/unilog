#include <iostream>
#include <SWI-Prolog.h>

#include "test_utils.hpp"

extern void test_variant_functions_main();
extern void test_lexer_main();
extern void test_parser_main();

void unit_test_main()
{
    constexpr bool ENABLE_DEBUG_LOGS = true;

    TEST(test_variant_functions_main);
    TEST(test_lexer_main);
    TEST(test_parser_main);
}

int main(int argc, char **argv)
{
    constexpr bool ENABLE_DEBUG_LOGS = true;

    const char *plav[] = {argv[0], "--quiet", "--nosignals"};

    /* initialise Prolog */
    if (!PL_initialise(3, const_cast<char **>(plav)))
        PL_halt(1);

    TEST(unit_test_main);

    PL_halt(0); // Properly halt the Prolog engine
    return 0;
}
