#ifndef VARIANT_FUNCTIONS_H
#define VARIANT_FUNCTIONS_H

#include <functional>
#include <istream>
#include <variant>

namespace unilog
{
    template <typename... Ts>
    std::istream &operator>>(std::istream &a_istream, std::variant<Ts...> &a_result)
    {
        std::streampos l_restore_point = a_istream.tellg();

        auto l_try_extract = [&a_istream, &a_result, l_restore_point](auto a_alternative)
        {
            if (a_istream >> a_alternative)
            {
                a_result = a_alternative;
                return true;
            }

            a_istream.clear();
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
