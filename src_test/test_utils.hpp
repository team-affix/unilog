#ifndef TEST_UTILS_H
#define TEST_UTILS_H

#include <assert.h>

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
