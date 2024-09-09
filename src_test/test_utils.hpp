#ifndef TEST_UTILS_H
#define TEST_UTILS_H

#include <assert.h>

#define LOG(x)             \
    if (ENABLE_DEBUG_LOGS) \
        std::cout << x;

#define TEST(void_fn)                                \
    LOG("TEST STARTING: " << #void_fn << std::endl); \
    void_fn();                                       \
    LOG("TEST FINISHED: " << #void_fn << std::endl);

#define assert_throws(void_fxn) \
    {                           \
        bool l_threw = false;   \
        try                     \
        {                       \
            void_fxn();         \
        }                       \
        catch (...)             \
        {                       \
            l_threw = true;     \
        }                       \
        assert(l_threw);        \
    }

#endif
