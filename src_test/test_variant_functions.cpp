#include <variant>
#include <sstream>
#include "../src_lib/variant_functions.hpp"
#include "test_utils.hpp"

void test_variant_2_types_extract()
{
    using unilog::operator>>;

    std::stringstream l_ss("1 1.1f 2.2 abc");

    std::variant<int, char> l_variant;

    l_ss >> l_variant; // 1
    assert(std::get<int>(l_variant) == 1);
    assert(!l_ss.fail());

    l_ss >> l_variant; // 1
    assert(std::get<int>(l_variant) == 1);
    assert(!l_ss.fail());

    l_ss >> l_variant; // .
    assert(std::get<char>(l_variant) == '.');
    assert(!l_ss.fail());

    l_ss >> l_variant; // 1
    assert(std::get<int>(l_variant) == 1);
    assert(!l_ss.fail());

    l_ss >> l_variant; // f
    assert(std::get<char>(l_variant) == 'f');
    assert(!l_ss.fail());

    l_ss >> l_variant; // 2
    assert(std::get<int>(l_variant) == 2);
    assert(!l_ss.fail());

    l_ss >> l_variant; // .
    assert(std::get<char>(l_variant) == '.');
    assert(!l_ss.fail());

    l_ss >> l_variant; // 2
    assert(std::get<int>(l_variant) == 2);
    assert(!l_ss.fail());

    l_ss >> l_variant; // a
    assert(std::get<char>(l_variant) == 'a');
    assert(!l_ss.fail());

    l_ss >> l_variant; // b
    assert(std::get<char>(l_variant) == 'b');
    assert(!l_ss.fail());

    l_ss >> l_variant; // c
    assert(std::get<char>(l_variant) == 'c');
    assert(!l_ss.fail());

    l_ss >> l_variant;
    assert(l_ss.fail());
    assert(l_ss.eof());
}

void test_variant_functions_main()
{
    constexpr bool ENABLE_DEBUG_LOGS = true;

    TEST(test_variant_2_types_extract);
}
