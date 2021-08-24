#ifndef MIX_DD_UTILS_HPP
#define MIX_DD_UTILS_HPP

#include "types.hpp"
#include <functional>
#include <vector>

namespace teddy::utils
{
    template<class F>
    concept i_gen = requires (F f, uint_t k)
    {
        std::invoke(f, k);
    };

    template<i_gen Gen>
    auto fill_vector (std::size_t const n, Gen&& f)
    {
        using T = decltype(std::invoke(f, uint_t {}));
        auto xs = std::vector<T>();
        xs.reserve(n);
        for (auto i = uint_t {0}; i < n; ++i)
        {
            xs.emplace_back(std::invoke(f, i));
        }
        return xs;
    }

    auto constexpr identity = [](auto const a) { return a; };
}

#endif