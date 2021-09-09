#ifndef MIX_DD_NODE_MANAGER_HPP
#define MIX_DD_NODE_MANAGER_HPP

#include "types.hpp"
#include "utils.hpp"
#include "node.hpp"
#include "node_pool.hpp"
#include "hash_tables.hpp"
#include "operators.hpp"

#include <algorithm>
#include <cassert>
#include <functional>
#include <span>
#include <utility>
#include <vector>

namespace teddy
{
    namespace domains
    {
        struct mixed
        {
            std::vector<uint_t> ds_;

            mixed (std::vector<uint_t> ds) :
                ds_ (std::move(ds))
            {
            };

            auto operator[] (uint_t const i) const
            {
                return ds_[i];
            }
        };

        template<uint_t N>
        struct fixed
        {
            static_assert(N > 1);
            constexpr auto operator[] (uint_t const) const
            {
                return N;
            }
        };

        template<class T>
        struct is_fixed : public std::false_type {};

        template<uint_t N>
        struct is_fixed<fixed<N>> : public std::true_type {};

        template<class T>
        using is_mixed = std::is_same<T, mixed>;
    }

    template<class T>
    concept domain = domains::is_mixed<T>()() or domains::is_fixed<T>()();

    template<class Data, degree Degree, domain Domain>
    class node_manager
    {
    public:
        using node_t      = node<Data, Degree>;
        using sons_t      = typename node_t::sons_t;
        using op_cache_t  = apply_cache<Data, Degree>;
        using op_cache_it = typename op_cache_t::iterator;
        struct common_init {};

    public:
        node_manager ( std::size_t vars
                     , std::size_t nodes
                     , std::vector<index_t> order )
                     requires(domains::is_fixed<Domain>()());

        node_manager ( std::size_t vars
                     , std::size_t nodes
                     , std::vector<index_t> order
                     , domains::mixed )
                     requires(domains::is_mixed<Domain>()());

        node_manager (node_manager&&) noexcept = default;
        node_manager (node_manager const&) = delete;

        auto set_cache_ratio  (std::size_t) -> void;
        auto set_pool_ratio   (std::size_t) -> void;
        auto set_auto_reorder (bool)        -> void;

    private:
        node_manager ( common_init
                     , std::size_t vars
                     , std::size_t nodes
                     , std::vector<index_t> order
                     , Domain );

    public:
        auto get_terminal_node (uint_t) const      -> node_t*;
        auto terminal_node     (uint_t)            -> node_t*;
        auto internal_node     (index_t, sons_t&&) -> node_t*; // tu si treba premyslieť, či to alokovať hneď, alebo sem posielať iba
        auto get_level         (index_t) const     -> level_t; // pointer na jedno spoločné miesto pre jeden apply a alokovať až pri nových
        auto get_level         (node_t*) const     -> level_t;
        auto get_node_count    (index_t) const     -> std::size_t;
        auto get_node_count    () const            -> std::size_t;
        auto get_var_count     () const            -> std::size_t;
        auto get_order         () const            -> std::span<index_t const>;
        auto get_domains       () const            -> std::vector<uint_t>;
        auto adjust_sizes      ()                  -> void;
        auto collect_garbage   ()                  -> void;

        template<utils::i_gen Gen>
        auto make_sons (index_t, Gen&&) -> sons_t;

        template<class NodeOp>
        auto for_each_son (node_t*, NodeOp&&) const -> void;

        template<class NodeOp>
        auto for_each_node (NodeOp&&) const -> void;

        template<class Op>
        auto cache_find (node_t*, node_t*) -> op_cache_it;

        template<class Op>
        auto cache_put (op_cache_it, node_t*, node_t*, node_t*) -> void;


            auto swap_vars        (index_t i)         -> void;
            auto sift_vars        ()                  -> void;

        // template<class VertexOp>
        // auto for_each_vertex (VertexOp op) const -> void;

        // template<class VertexOp>
        // auto for_each_terminal_node (VertexOp op) const -> void;

        auto is_valid (index_t, uint_t) const -> bool;

        static auto inc_ref_count (node_t* v) -> node_t*;
        static auto dec_ref_count (node_t* v) -> void;

    private:
        using unique_table_t = unique_table<Data, Degree>;
        using unique_table_v = std::vector<unique_table_t>;
        using op_cache_a     = std::array<op_cache_t, OpCount>;

    private:
        auto node_hash    (index_t, sons_t const&) const -> std::size_t;
        auto node_equal   (node_t*, sons_t const&) const -> bool;
        auto is_redundant (index_t, sons_t const&) const -> bool;
        auto get_index    (level_t) const                -> index_t;

        template<class... Args>
        auto new_node (Args&&...) -> node_t*;
        auto delete_node (node_t* v) -> void;

        static auto check_distinct (std::vector<index_t> const&) -> bool;


            auto swap_vertex    (node_t* v) -> void;
            auto dec_ref_try_gc (node_t* v) -> void;

    private:
        unique_table_v          uniqueTables_;
        std::vector<node_t*>    terminals_;
        std::vector<level_t>    indexToLevel_;
        std::vector<index_t>    levelToIndex_;
        op_cache_a              opCaches_;
        node_pool<Data, Degree> pool_;
        bool                    needsGc_;
        std::size_t             nodeCount_;
        std::size_t             cacheRatio_;
        bool                    reorderEnabled_;

        [[no_unique_address]]
        Domain               domains_;
    };

    template<class Data, degree D>
    auto node_value (node<Data, D>* const n) -> uint_t
    {
        return n->is_terminal() ? n->get_value() : Nondetermined;
    }

    template<class Data, degree Degree, domain Domain>
    node_manager<Data, Degree, Domain>::node_manager
        ( std::size_t const    vars
        , std::size_t const    nodes
        , std::vector<index_t> order )
        requires(domains::is_fixed<Domain>()()) :
        node_manager ( common_init()
                     , vars
                     , nodes
                     , std::move(order)
                     , {} )
    {
    }

    template<class Data, degree Degree, domain Domain>
    node_manager<Data, Degree, Domain>::node_manager
        ( std::size_t const    vars
        , std::size_t const    nodes
        , std::vector<index_t> order
        , domains::mixed       ds )
        requires(domains::is_mixed<Domain>()()) :
        node_manager ( common_init()
                     , vars
                     , nodes
                     , std::move(order)
                     , std::move(ds) )
    {
    }

    template<class Data, degree Degree, domain Domain>
    node_manager<Data, Degree, Domain>::node_manager
        ( common_init
        , std::size_t const    vars
        , std::size_t const    nodes
        , std::vector<index_t> order
        , Domain               ds ) :
        uniqueTables_   (vars),
        terminals_      ({}),
        indexToLevel_   (vars),
        levelToIndex_   (std::move(order)),
        opCaches_       ({}),
        pool_           (nodes),
        needsGc_        (false),
        nodeCount_      (0),
        cacheRatio_     (4),
        reorderEnabled_ (false),
        domains_        (std::move(ds))
    {
        assert(levelToIndex_.size() == this->get_var_count());
        assert(check_distinct(levelToIndex_));
        if constexpr ( domains::is_mixed<Domain>()()
                   and degrees::is_fixed<Degree>()() )
        {
            for (auto const d : domains_.ds_)
            {
                assert(d < Degree()());
            }
        }

        auto level = 0u;
        for (auto const index : levelToIndex_)
        {
            indexToLevel_[index] = level++;
        }
    }

    template<class Data, degree Degree, domain Domain>
    auto node_manager<Data, Degree, Domain>::set_cache_ratio
        (std::size_t const denom) -> void
    {
        assert(denom > 0);
        cacheRatio_ = denom;
    }

    template<class Data, degree Degree, domain Domain>
    auto node_manager<Data, Degree, Domain>::set_pool_ratio
        (std::size_t const denom) -> void
    {
        assert(denom > 0);
        pool_.set_overflow_ratio(denom);
    }

    template<class Data, degree Degree, domain Domain>
    auto node_manager<Data, Degree, Domain>::set_auto_reorder
        (bool const reorder) -> void
    {
        reorderEnabled_ = reorder;
    }

    template<class Data, degree Degree, domain Domain>
    auto node_manager<Data, Degree, Domain>::get_terminal_node
        (uint_t const v) const -> node_t*
    {
        return v < terminals_.size() ? terminals_[v] : nullptr;
    }

    template<class Data, degree Degree, domain Domain>
    auto node_manager<Data, Degree, Domain>::terminal_node
        (uint_t const v) -> node_t*
    {
        if (v >= terminals_.size())
        {
            terminals_.resize(v + 1, nullptr);
        }

        if (not terminals_[v])
        {
            terminals_[v] = this->new_node(v);
        }

        return terminals_[v];
    }

    template<class Data, degree Degree, domain Domain>
    auto node_manager<Data, Degree, Domain>::internal_node
        (index_t const i, sons_t&& sons) -> node_t*
    {
        if (this->is_redundant(i, sons))
        {
            return sons[0];
        }

        auto const eq = std::bind_front(&node_manager::node_equal, this);

        auto& table         = uniqueTables_[i];
        auto const hash     = this->node_hash(i, sons);
        auto const existing = table.find(sons, hash, eq);
        if (existing)
        {
            return existing;
        }

        auto n = this->new_node(i, std::move(sons));
        table.insert(n, hash);
        this->for_each_son(n, inc_ref_count);

        return n;
    }

    template<class Data, degree Degree, domain Domain>
    auto node_manager<Data, Degree, Domain>::get_level
        (index_t const i) const -> level_t
    {
        return indexToLevel_[i];
    }

    template<class Data, degree Degree, domain Domain>
    auto node_manager<Data, Degree, Domain>::get_level
        (node_t* const n) const -> level_t
    {
        return n->is_terminal()
                   ? TerminalLevel
                   : this->get_level(n->get_index());
    }

    template<class Data, degree Degree, domain Domain>
    auto node_manager<Data, Degree, Domain>::get_node_count
        (index_t const i) const -> std::size_t
    {
        return uniqueTables_[i].size();
    }

    template<class Data, degree Degree, domain Domain>
    auto node_manager<Data, Degree, Domain>::get_node_count
        () const -> std::size_t
    {
        return nodeCount_;
    }

    template<class Data, degree Degree, domain Domain>
    auto node_manager<Data, Degree, Domain>::get_var_count
        () const -> std::size_t
    {
        return uniqueTables_.size();
    }

    template<class Data, degree Degree, domain Domain>
    auto node_manager<Data, Degree, Domain>::get_order
        () const -> std::span<index_t const>
    {
        auto const first = levelToIndex_.data();
        auto const last  = levelToIndex_.data() + levelToIndex_.size();
        return std::span<index_t const>(first, last);
    }

    template<class Data, degree Degree, domain Domain>
    auto node_manager<Data, Degree, Domain>::get_domains
        () const -> std::vector<uint_t>
    {
        auto ds = std::vector<uint_t>(this->get_var_count());
        std::generate_n(std::begin(ds), ds.size(), [this, k = 0u]() mutable
        {
            return domains_[k++];
        });
        return ds;
    }

    template<class Data, degree Degree, domain Domain>
    auto node_manager<Data, Degree, Domain>::adjust_sizes
        () -> void
    {
        if (needsGc_)
        {
            this->collect_garbage();
            if (reorderEnabled_)
            {
                this->sift_vars();
            }
        }

        auto const hash = std::bind_front(&node_manager::node_hash, this);
        for (auto& t : uniqueTables_)
        {
            t.adjust_capacity(hash);
        }

        for (auto& c : opCaches_)
        {
            c.adjust_capacity(nodeCount_ / cacheRatio_);
        }
    }

    template<class Data, degree Degree, domain Domain>
    auto node_manager<Data, Degree, Domain>::collect_garbage
        () -> void
    {
        needsGc_ = false;

        for (auto& table : uniqueTables_)
        {
            auto const end = std::end(table);
            auto it = std::begin(table);

            while (it != end)
            {
                auto const n = *it;
                if (0 == n->get_ref_count())
                {
                    this->for_each_son(n, dec_ref_count);
                    it = table.erase(it);
                    this->delete_node(n);
                }
                else
                {
                    ++it;
                }
            }
        }
    }

    template<class Data, degree Degree, domain Domain>
    template<utils::i_gen Gen>
    auto node_manager<Data, Degree, Domain>::make_sons
        (index_t const i, Gen&& g) -> sons_t
    {
        auto ss = node_t::container(domains_[i], Degree());
        for (auto k = uint_t {0}; k < domains_[i]; ++k)
        {
            ss[k] = std::invoke(g, k);
        }
        return ss;
    }

    template<class Data, degree Degree, domain Domain>
    template<class NodeOp>
    auto node_manager<Data, Degree, Domain>::for_each_son
        (node_t* const node, NodeOp&& f) const -> void
    {
        auto const i = node->get_index();
        for (auto k = 0u; k < domains_[i]; ++k)
        {
            std::invoke(f, node->get_son(k));
        }
    }

    template<class Data, degree Degree, domain Domain>
    template<class NodeOp>
    auto node_manager<Data, Degree, Domain>::for_each_node
        (NodeOp&& f) const -> void
    {
        for (auto const& table : uniqueTables_)
        {
            for (auto const n : table)
            {
                std::invoke(f, n);
            }
        }

        for (auto const n : terminals_)
        {
            if (n)
            {
                std::invoke(f, n);
            }
        }
    }

    template<class Data, degree Degree, domain Domain>
    template<class Op>
    auto node_manager<Data, Degree, Domain>::cache_find
        (node_t* const l, node_t* const r) -> op_cache_it
    {
        auto& cache = opCaches_[op_id(Op())];
        return cache.find(l, r);
    }

    template<class Data, degree Degree, domain Domain>
    template<class Op>
    auto node_manager<Data, Degree, Domain>::cache_put
        ( op_cache_it it
        , node_t* const l
        , node_t* const r
        , node_t* const res ) -> void
    {
        auto& cache = opCaches_[op_id(Op())];
        cache.put(it, l, r, res);
    }

    template<class Data, degree Degree, domain Domain>
    auto node_manager<Data, Degree, Domain>::is_valid
        (index_t const i, uint_t const v) const -> bool
    {
        return v < domains_[i];
    }















    template<class Data, degree Degree, domain Domain>
    auto node_manager<Data, Degree, Domain>::inc_ref_count
        (node_t* const v) -> node_t*
    {
        v->inc_ref_count();
        return v;
    }

    template<class Data, degree Degree, domain Domain>
    auto node_manager<Data, Degree, Domain>::dec_ref_count
        (node_t* const v) -> void
    {
        v->dec_ref_count();
    }



    template<class Data, degree Degree, domain Domain>
    auto node_manager<Data, Degree, Domain>::node_hash
        (index_t const i, sons_t const& ss) const -> std::size_t
    {
        auto result = std::size_t(0);
        for (auto j = 0u; j < domains_[i]; ++j)
        {
            auto const hash = std::hash<node_t*>()(ss[j]);
            result ^= hash + 0x9e3779b9 + (result << 6) + (result >> 2);
        }
        return result;
    }

    template<class Data, degree Degree, domain Domain>
    auto node_manager<Data, Degree, Domain>::node_equal
        (node_t* const n, sons_t const& ss) const -> bool
    {
        auto const i = n->get_index();
        for (auto j = 0u; j < domains_[i]; ++j)
        {
            if (n->get_son(j) != ss[j])
            {
                return false;
            }
        }
        return true;
    }

    template<class Data, degree Degree, domain Domain>
    auto node_manager<Data, Degree, Domain>::is_redundant
        (index_t const i, sons_t const& sons) const -> bool
    {
        for (auto j = 1u; j < domains_[i]; ++j)
        {
            if (sons[j - 1] != sons[j])
            {
                return false;
            }
        }
        return true;
    }

    template<class Data, degree Degree, domain Domain>
    auto node_manager<Data, Degree, Domain>::get_index
        (level_t const l) const -> index_t
    {
        return levelToIndex_[l];
    }

    template<class Data, degree Degree, domain Domain>
    template<class... Args>
    auto node_manager<Data, Degree, Domain>::new_node
        (Args&&... args) -> node_t*
    {
        ++nodeCount_;
        auto const n = pool_.try_create(std::forward<Args>(args)...);
        if (n)
        {
            return n;
        }

        needsGc_ = true;

        return pool_.force_create(std::forward<Args>(args)...);
    }

    template<class Data, degree Degree, domain Domain>
    auto node_manager<Data, Degree, Domain>::delete_node
        (node_t* const n) -> void
    {
        --nodeCount_;
        n->set_unused();
        pool_.destroy(n);
    }

    template<class Data, degree Degree, domain Domain>
    auto node_manager<Data, Degree, Domain>::check_distinct
        (std::vector<index_t> const& is) -> bool
    {
        auto const me = *std::max_element(std::begin(is), std::end(is));
        auto in = std::vector<bool>(me + 1, false);
        for (auto const i : is)
        {
            if (in[i])
            {
                return false;
            }
            in[i] = true;
        }
        return true;
    }














            template<class Data, degree Degree, domain Domain>
            auto node_manager<Data, Degree, Domain>::swap_vertex
                (node_t* const) -> void
            {
                // auto const index     = v->get_index();
                // auto const nextIndex = this->get_index(1 + this->get_vertex_level(v));
                // auto const vDomain   = this->get_domain(index);
                // auto const sonDomain = this->get_domain(nextIndex);
                // auto const oldSons   = utils::fill_vector(vDomain, std::bind_front(&node_t::get_son, v));
                // auto const cofactors = utils::fill_array_n<P>(vDomain, [=](auto const sonIndex)
                // {
                //     auto const son = v->get_son(sonIndex);
                //     return son->get_index() == nextIndex
                //         ? utils::fill_array_n<P>(sonDomain, std::bind_front(&node_t::get_son, son))
                //         : utils::fill_array_n<P>(sonDomain, utils::constant(son));
                // });
                // v->set_index(nextIndex);
                // v->set_sons(utils::fill_array_n<P>(sonDomain, [=, this, &cofactors](auto const i)
                // {
                //     return this->internal_node(index, utils::fill_array_n<P>(vDomain, [=, &cofactors](auto const j)
                //     {
                //         return cofactors[j][i];
                //     }));
                // }));
                // v->for_each_son(inc_ref_count);
                // utils::for_all(oldSons, std::bind_front(&node_manager::dec_ref_try_gc, this));
            }


            template<class Data, degree Degree, domain Domain>
            auto node_manager<Data, Degree, Domain>::dec_ref_try_gc
                (node_t* const) -> void
            {
                // v->dec_ref_count();

                // if (v->get_ref_count() > 0 || this->is_leaf_vertex(v))
                // {
                //     return;
                // }

                // v->for_each_son(std::bind_front(&node_manager::dec_ref_try_gc, this));
                // uniqueTables_[v->get_index()].erase(v);
                // this->delete_node(v);
            }



        template<class Data, degree Degree, domain Domain>
        auto node_manager<Data, Degree, Domain>::swap_vars
            (index_t const) -> void
        {
            // auto const iLevel    = this->get_level(i);
            // auto const nextIndex = this->get_index(1 + iLevel);
            // auto tmpTable        = unique_table_t(std::move(uniqueTables_[i]));
            // for (auto const v : tmpTable)
            // {
            //     this->swap_vertex(v);
            // }
            // uniqueTables_[i].adjust_capacity();
            // uniqueTables_[nextIndex].merge(tmpTable);

            // std::swap(levelToIndex_[iLevel], levelToIndex_[1 + iLevel]);
            // ++indexToLevel_[i];
            // --indexToLevel_[nextIndex];
        }

        template<class Data, degree Degree, domain Domain>
        auto node_manager<Data, Degree, Domain>::sift_vars
            () -> void
        {
            // using count_pair = struct { index_t index; std::size_t count; };

            // auto const get_sift_order = [this]()
            // {
            //     auto counts = utils::fill_vector(this->get_var_count(), [this](auto const i)
            //     {
            //         return count_pair {i, this->get_node_count(i)};
            //     });
            //     std::sort(std::begin(counts), std::end(counts), [](auto&& l, auto&& r){ return l.count > r.count; });
            //     return counts;
            // };

            // auto const move_var_down = [this](auto const index)
            // {
            //     this->swap_vars(index);
            // };

            // auto const move_var_up = [this](auto const index)
            // {
            //     auto const level     = this->get_level(index);
            //     auto const prevIndex = this->get_index(level - 1);
            //     this->swap_vars(prevIndex);
            // };

            // auto const place_variable = [&, this](auto const index)
            // {
            //     auto level        = this->get_level(index);
            //     auto optimalLevel = level;
            //     auto optimalCount = nodeCount_;

            //     // Sift down.
            //     while (level != this->get_last_internal_level())
            //     {
            //         move_var_down(index);
            //         ++level;
            //         if (nodeCount_ < optimalCount)
            //         {
            //             optimalCount = nodeCount_;
            //             optimalLevel = level;
            //         }
            //     }

            //     // Sift up.
            //     while (level != 0)
            //     {
            //         move_var_up(index);
            //         --level;
            //         if (nodeCount_ < optimalCount)
            //         {
            //             optimalCount = nodeCount_;
            //             optimalLevel = level;
            //         }
            //     }

            //     // Restore optimal position.
            //     while (level != optimalLevel)
            //     {
            //         move_var_down(index);
            //         ++level;
            //     }
            // };

            // auto const siftOrder = get_sift_order();
            // for (auto pair : siftOrder)
            // {
            //     place_variable(pair.index);
            // }
        }

            // template<class Data, degree Degree, domain Domain>
            // template<class VertexOp>
            // auto node_manager<Data, Degree, Domain>::for_each_vertex
            //     (VertexOp op) const -> void
            // {
            //     for (auto const& table : uniqueTables_)
            //     {
            //         auto const end = std::end(table);
            //         auto it = std::begin(table);
            //         while (it != end)
            //         {
            //             auto const v = *it;
            //             ++it;
            //             op(v);
            //         }
            //     }

            //     for (auto const v : terminals_)
            //     {
            //         if (v)
            //         {
            //             op(v);
            //         }
            //     }
            // }

            // template<class Data, degree Degree, domain Domain>
            // template<class VertexOp>
            // auto node_manager<Data, Degree, Domain>::for_each_terminal_node
            //     (VertexOp op) const -> void
            // {
            //     for (auto const v : terminals_)
            //     {
            //         if (v)
            //         {
            //             op(v);
            //         }
            //     }
            // }
}

#endif