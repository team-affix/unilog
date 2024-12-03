#ifndef TEST_UTILS_H
#define TEST_UTILS_H

#include <assert.h>
#include <utility>
#include <iostream>
#include <vector>

#define LOG(x)             \
    if (ENABLE_DEBUG_LOGS) \
        std::cout << x;

#define TEST(void_fn)                                     \
    LOG(">>>> TEST STARTING: " << #void_fn << std::endl); \
    void_fn();                                            \
    LOG("    <<<< TEST FINISHED: " << #void_fn << std::endl);

template <typename Key, typename Value>
using data_points = std::vector<std::pair<Key, Value>>;

#endif
