#pragma once

#include <stdint.h>
#include <iostream>
#include <limits>

namespace star
{
class CastHelpers
{
  public:
    static uint32_t size_t_to_unsigned_int(size_t org)
    {
        if (org > std::numeric_limits<uint32_t>::max())
        {
            std::cerr << "Error: Size too large for uint32_t" << std::endl;
            exit(1);
        }

        return static_cast<uint32_t>(org);
    }

    template <typename In, typename Out> static bool SafeCast(const In &in, Out &out)
    {
        if constexpr (std::is_integral_v<In> && std::is_integral_v<Out>)
        {
            if (in >= static_cast<In>(std::numeric_limits<Out>::min()) &&
                in <= static_cast<In>(std::numeric_limits<Out>::max()))
            {
                out = static_cast<Out>(in);
                return true;
            }
            else
            {
                return false; // Out of range
            }
        }
        else
        {
            static_assert(std::is_integral_v<In> && std::is_integral_v<Out>, "SafeCast only supports integral types.");
        }
    }
};
} // namespace star