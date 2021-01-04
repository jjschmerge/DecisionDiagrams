#ifndef MIX_DD_MDD_MANAGER_HPP
#include "../mdd_manager.hpp"
#endif

namespace mix::dd
{
    template<class VertexData, class ArcData, std::size_t P>
    auto mdd_manager<VertexData, ArcData, P>::just_val
        (log_t const val) -> mdd_t
    {
        return mdd_t {vertexManager_.terminal_vertex(val)};
    }

    template<class VertexData, class ArcData, std::size_t P>
    auto mdd_manager<VertexData, ArcData, P>::just_var
        (index_t const i) -> mdd_t
    {
        auto constexpr ND = log_val_traits<P>::nodomain;
        return this->just_var_impl(i, utils::fill_array<P>([=, this](auto const val)
        {
            return val < this->get_domain(i) ? val : ND;
        }));
    }

    template<class VertexData, class ArcData, std::size_t P>
    auto mdd_manager<VertexData, ArcData, P>::operator()
        (index_t const i) -> mdd_t
    {
        return this->just_var(i);
    }

    template<class VertexData, class ArcData, std::size_t P>
    template<class LeafVals>
    auto mdd_manager<VertexData, ArcData, P>::just_var_impl
        (index_t const i, LeafVals&& vals) -> mdd_t
    {
        auto const leaves = utils::map_to_array<P>(vals, [this](auto const lv)
        {
            return vertexManager_.terminal_vertex(static_cast<log_t>(lv));
        });
        return mdd_t {vertexManager_.internal_vertex(i, leaves)};
    }
}