#ifndef MIX_UTILS_MORE_TYPE_TRAITS_
#define MIX_UTILS_MORE_TYPE_TRAITS_

#include <array>
#include <bitset>
#include <vector>

namespace mix::utils
{
    /**
        Provides member constant that is equal to true if the
        T is std::array and is false otherwise.
     */
    template<class T>
    struct is_std_array : public std::false_type {};

    template<class T, std::size_t N>
    struct is_std_array<std::array<T, N>> : public std::true_type {};

    template<class T>
    inline constexpr auto is_std_array_v = is_std_array<T>::value;

    /**
        Provides member constant that is equal to true if the
        type of T is std::vector and is false otherwise.
     */
    template<class T>
    struct is_std_vector : public std::false_type {};

    template<class T, class Allocator>
    struct is_std_vector<std::vector<T, Allocator>> : public std::true_type {};

    template<class T>
    inline constexpr auto is_std_vector_v = is_std_vector<T>::value;

    /**
        Provides member constant that is equal to true if the
        type of T is std::bitset and is false otherwise.
     */
    template<class T>
    struct is_std_bitset : public std::false_type {};

    template<std::size_t N>
    struct is_std_bitset<std::bitset<N>> : public std::true_type {};

    template<class T>
    inline constexpr auto is_std_bitset_v = is_std_bitset<T>::value;

    /**
        Provides member constant that is equal to true if the
        Iterator is a random access iterator.
     */
    template<class Iterator>
    struct is_random_access : public std::conditional_t< std::is_same_v< std::random_access_iterator_tag
                                                                       , typename std::iterator_traits<Iterator>::iterator_category >
                                                       , std::true_type
                                                       , std::false_type > {};

    template<class Iterator>
    inline constexpr auto is_random_access_v = is_random_access<Iterator>::value;


    /**
        Provides member constant `value` that is equal to number
        of bits that represents value of given type.
     */
    template<class T>
    struct bit_count : std::integral_constant<std::size_t, 8 * sizeof(T)> {};

    template<class T>
    inline constexpr auto bit_count_v = bit_count<T>::value;

    namespace types_impl
    {
        // https://stackoverflow.com/questions/81870/is-it-possible-to-print-a-variables-type-in-standard-c

        template <class T>
        inline constexpr auto type_name() -> std::string_view
        {
        using namespace std;
        #ifdef __clang__
            string_view p = __PRETTY_FUNCTION__;
            return string_view(p.data() + 34, p.size() - 34 - 1);
        #elif defined(__GNUC__)
            string_view p = __PRETTY_FUNCTION__;
        #  if __cplusplus < 201402
            return string_view(p.data() + 36, p.size() - 36 - 1);
        #  else
            return string_view(p.data() + 49, p.find(';', 49) - 49);
        #  endif
        #elif defined(_MSC_VER)
            string_view p = __FUNCSIG__;
            return string_view(p.data() + 84, p.size() - 84 - 7);
        #endif
        }
    }
}

#endif