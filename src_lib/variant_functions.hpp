#ifndef VARIANT_FUNCTIONS_HPP
#define VARIANT_FUNCTIONS_HPP

#include <functional>
#include <istream>
#include <variant>

#include <iostream>

namespace unilog
{
    template <typename... Ts>
    std::istream &operator>>(std::istream &a_istream, std::variant<Ts...> &a_result)
    {
        // here, peek() will look ahead for eof, and set eofbit correspondingly
        if (a_istream.peek(), !a_istream.good())
        {
            a_istream.setstate(std::ios::failbit);
            return a_istream;
        }

        std::streampos l_restore_point = a_istream.tellg();

        auto l_try_extract = [&a_istream, &a_result, l_restore_point](auto a_alternative)
        {
            if (a_istream >> a_alternative)
            {
                a_result = a_alternative;
                return true;
            }

            a_istream.clear();                // clear all flags when restoring
            a_istream.seekg(l_restore_point); // Restore point for next attempt

            return false;
        };

        if ((l_try_extract(Ts{}) || ...))
            return a_istream; // Return immediately if any type succeeds

        a_istream.setstate(std::ios::failbit); // No types matched, set failbit

        return a_istream;
    }
}

#endif
