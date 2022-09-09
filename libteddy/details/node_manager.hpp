#ifndef LIBTEDDY_DETAILS_NODE_MANAGER_HPP
#define LIBTEDDY_DETAILS_NODE_MANAGER_HPP

#include <algorithm>
#include <cassert>
#include <cstdint>
#include <functional>
#include <libteddy/details/debug.hpp>
#include <libteddy/details/hash_tables.hpp>
#include <libteddy/details/node.hpp>
#include <libteddy/details/node_pool.hpp>
#include <libteddy/details/operators.hpp>
#include <libteddy/details/utils.hpp>
#include <queue>
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

    mixed(std::vector<uint_t> ds) : ds_(std::move(ds)) {};

    auto operator[](uint_t const i) const { return ds_[i]; }
};

template<uint_t N>
struct fixed
{
    static_assert(N > 1);
    constexpr auto operator[](uint_t const) const { return N; }

    constexpr auto operator()() const { return N; }
};

template<class T>
struct is_fixed : public std::false_type
{
};

template<uint_t N>
struct is_fixed<fixed<N>> : public std::true_type
{
};

template<class T>
using is_mixed = std::is_same<T, mixed>;
} // namespace domains

template<class T>
concept domain = domains::is_mixed<T>()() || domains::is_fixed<T>()();

// TODO spravit jeden concept mapper<From, To, F>
template<class F>
concept uint_to_bool = requires(F f, uint_t x) {
                           {
                               f(x)
                               } -> std::convertible_to<bool>;
                       };

template<class F>
concept uint_to_uint = requires(F f, uint_t x) {
                           {
                               f(x)
                               } -> std::convertible_to<uint_t>;
                       };

template<class F, class Node>
concept node_op = requires(F f, Node* node) { f(node); };

template<class Data, degree Degree, domain Domain>
class node_manager
{
public:
    using node_t     = node<Data, Degree>;
    using sons_t     = typename node_t::sons_t;
    using op_cache_t = apply_cache<Data, Degree>;
    struct common_init
    {
    };

public:
    node_manager(
        std::size_t varCount, std::size_t nodePoolSize,
        std::size_t overflowNodePoolSize, std::vector<index_t> order
    )
    requires(domains::is_fixed<Domain>()());

    node_manager(
        std::size_t varCount, std::size_t nodePoolSize,
        std::size_t overflowNodePoolSize, std::vector<index_t> order,
        domains::mixed
    )
    requires(domains::is_mixed<Domain>()());

    node_manager(node_manager&&) noexcept = default;
    node_manager(node_manager const&)     = delete;

    auto set_cache_ratio(double) -> void;
    auto set_gc_ratio(double) -> void;
    auto set_auto_reorder(bool) -> void;

private:
    node_manager(
        common_init, std::size_t varCount, std::size_t nodePoolSize,
        std::size_t overflowNodePoolSize, std::vector<index_t> order, Domain
    );

public:
    auto get_terminal_node(uint_t) const -> node_t*;
    auto terminal_node(uint_t) -> node_t*;
    auto special_node(uint_t) -> node_t*;
    auto internal_node(index_t, sons_t&&) -> node_t*;
    auto get_level(index_t) const -> level_t;
    auto get_level(node_t*) const -> level_t;
    auto get_leaf_level() const -> level_t;
    auto get_index(level_t) const -> index_t;
    auto get_domain(index_t) const -> uint_t;
    auto get_node_count(index_t) const -> std::size_t;
    auto get_node_count(node_t*) const -> std::size_t;
    auto get_node_count() const -> std::size_t;
    auto get_var_count() const -> std::size_t;
    auto get_order() const -> std::vector<index_t> const&;
    auto get_domains() const -> std::vector<uint_t>;
    auto force_gc() -> void;

    auto to_dot_graph(std::ostream&) const -> void;
    auto to_dot_graph(std::ostream&, node_t*) const -> void;

    auto domain_product(level_t, level_t) const -> std::size_t;

    template<utils::i_gen F>
    auto make_sons(index_t, F&&) -> sons_t;

    template<node_op<node<Data, Degree>> NodeOp>
    auto for_each_son(node_t*, NodeOp&&) const -> void;

    template<node_op<node<Data, Degree>> NodeOp>
    auto for_each_son(index_t, sons_t const&, NodeOp&&) const -> void;

    template<node_op<node<Data, Degree>> NodeOp>
    auto for_each_node(NodeOp&&) const -> void;

    template<node_op<node<Data, Degree>> NodeOp>
    auto for_each_terminal_node(NodeOp&&) const -> void;

    template<bin_op O>
    auto cache_find(node_t*, node_t*) -> node_t*;

    template<bin_op O>
    auto cache_put(node_t*, node_t*, node_t*) -> void;

    template<node_op<node<Data, Degree>> NodeOp>
    auto traverse_pre(node_t*, NodeOp&&) const -> void;

    template<node_op<node<Data, Degree>> NodeOp>
    auto traverse_post(node_t*, NodeOp&&) const -> void;

    template<node_op<node<Data, Degree>> NodeOp>
    auto traverse_level(node_t*, NodeOp&&) const -> void;

    auto is_valid_var_value(index_t, uint_t) const -> bool;

    auto run_deferred() -> void;

    static auto dec_ref_count(node_t*) -> void;

    auto sift_variables() -> void;

private:
    auto is_redundant(index_t, sons_t const&) const -> bool;

    auto adjust_tables() -> void;
    auto adjust_caches() -> void;

    auto swap_variable_with_next(index_t) -> void;
    auto swap_node_with_next(node_t*) -> void;
    auto dec_ref_try_gc(node_t*) -> void;

    template<class... Args>
    auto new_node(Args&&...) -> node_t*;
    auto delete_node(node_t*) -> void;

    auto traverse_no_op(node_t*) const -> void;

    template<class ForEachNode>
    auto to_dot_graph_common(std::ostream&, ForEachNode&&) const -> void;

    auto deferr_gc_reorder() -> void;

    auto collect_garbage() -> void;

    static auto check_distinct(std::vector<index_t> const&) -> bool;

    static auto can_be_gced(node_t*) -> bool;

private:
    using unique_table_t = unique_table<Data, Degree>;

private:
    apply_cache<Data, Degree> opCache_;
    node_pool<Data, Degree> pool_;
    std::vector<unique_table_t> uniqueTables_;
    std::vector<node_t*> terminals_;
    std::vector<node_t*> specials_;
    std::vector<level_t> indexToLevel_;
    std::vector<index_t> levelToIndex_;
    [[no_unique_address]] Domain domains_;
    std::size_t nodeCount_;
    double cacheRatio_;
    double gcRatio_;
    std::size_t nextTableAdjustment_;
    bool autoReorderEnabled_;
    bool gcReorderDeferred_;
};

template<class Data, degree D>
auto node_value(node<Data, D>* const n) -> uint_t
{
    return n->is_terminal() ? n->get_value() : Nondetermined;
}

template<class Data, degree D>
auto id_inc_ref_count(node<Data, D>* const n) -> node<Data, D>*
{
    n->inc_ref_count();
    return n;
}

template<class Data, degree D>
auto id_set_marked(node<Data, D>* const n) -> node<Data, D>*
{
    n->set_marked();
    return n;
}

template<class Data, degree D>
auto id_set_notmarked(node<Data, D>* const n) -> node<Data, D>*
{
    n->set_notmarked();
    return n;
}

template<class Data, degree Degree, domain Domain>
node_manager<Data, Degree, Domain>::node_manager(
    std::size_t const varCount, std::size_t const nodePoolSize,
    std::size_t const overflowNodePoolSize, std::vector<index_t> order
)
requires(domains::is_fixed<Domain>()())
    : node_manager(
          common_init(), varCount, nodePoolSize, overflowNodePoolSize,
          std::move(order), {}
      )
{
}

template<class Data, degree Degree, domain Domain>
node_manager<Data, Degree, Domain>::node_manager(
    std::size_t const varCount, std::size_t const nodePoolSize,
    std::size_t const overflowNodePoolSize, std::vector<index_t> order,
    domains::mixed domains
)
requires(domains::is_mixed<Domain>()())
    : node_manager(
          common_init(), varCount, nodePoolSize, overflowNodePoolSize,
          std::move(order),
          [&]() -> decltype(auto)
          {
              assert(domains.ds_.size() == varCount);
              return std::move(domains);
          }()
      )
{
}

template<class Data, degree Degree, domain Domain>
node_manager<Data, Degree, Domain>::node_manager(
    common_init, std::size_t const varCount, std::size_t const nodePoolSize,
    std::size_t const overflowNodePoolSize, std::vector<index_t> order,
    Domain domains
)
    : opCache_(), pool_(nodePoolSize, overflowNodePoolSize),
      uniqueTables_(varCount), terminals_({}), specials_({}),
      indexToLevel_(varCount), levelToIndex_(std::move(order)),
      domains_(std::move(domains)), nodeCount_(0), cacheRatio_(0.5),
      gcRatio_(0.05), nextTableAdjustment_(230), autoReorderEnabled_(false),
      gcReorderDeferred_(false)
{
    assert(levelToIndex_.size() == this->get_var_count());
    assert(check_distinct(levelToIndex_));
    if constexpr (domains::is_mixed<Domain>()() && degrees::is_fixed<Degree>()())
    {
        for ([[maybe_unused]] auto const d : domains_.ds_)
        {
            assert(d <= Degree()());
        }
    }

    // Create reverse mapping
    // from (level_t -> index_t)
    // to   (index_t -> level_t).
    auto level = 0u;
    for (auto const index : levelToIndex_)
    {
        indexToLevel_[index] = level++;
    }
}

template<class Data, degree Degree, domain Domain>
auto node_manager<Data, Degree, Domain>::set_cache_ratio(double const ratio)
    -> void
{
    assert(ratio > 0);
    cacheRatio_ = ratio;
}

template<class Data, degree Degree, domain Domain>
auto node_manager<Data, Degree, Domain>::set_gc_ratio(double const ratio)
    -> void
{
    assert(ratio >= 0.0 && ratio <= 1.0);
    gcRatio_ = ratio;
}

template<class Data, degree Degree, domain Domain>
auto node_manager<Data, Degree, Domain>::set_auto_reorder(bool const reorder)
    -> void
{
    autoReorderEnabled_ = reorder;
}

template<class Data, degree Degree, domain Domain>
auto node_manager<Data, Degree, Domain>::get_terminal_node(uint_t const v) const
    -> node_t*
{
    return v < terminals_.size() ? terminals_[v] : nullptr;
}

template<class Data, degree Degree, domain Domain>
auto node_manager<Data, Degree, Domain>::terminal_node(uint_t const v)
    -> node_t*
{
    if (is_special(v))
    {
        return this->special_node(v);
    }

    if constexpr (domains::is_fixed<Domain>()())
    {
        assert(v < Domain()());
    }

    if (v >= terminals_.size())
    {
        terminals_.resize(v + 1, nullptr);
    }

    if (not terminals_[v])
    {
        terminals_[v] = this->new_node(v);
    }

    return id_set_marked(terminals_[v]);
}

template<class Data, degree Degree, domain Domain>
auto node_manager<Data, Degree, Domain>::special_node(
    [[maybe_unused]] uint_t const v
) -> node_t*
{
    assert(v == Undefined);

    if (specials_.empty())
    {
        specials_.resize(1, nullptr);
    }

    if (not specials_[0])
    {
        specials_[0] = this->new_node(Undefined);
    }

    return id_set_marked(specials_[0]);
}

template<class Data, degree Degree, domain Domain>
auto node_manager<Data, Degree, Domain>::internal_node(
    index_t const i, sons_t&& sons
) -> node_t*
{
    // Each node comming out of here is marked.
    // Later on it must become son of someone or root of a diagram.
    auto ret = static_cast<node_t*>(nullptr);

    if (this->is_redundant(i, sons))
    {
        ret = sons[0];
    }
    else
    {
        auto& table                 = uniqueTables_[i];
        auto const [existing, hash] = table.find(sons, domains_[i]);
        if (existing)
        {
            ret = existing;
        }
        else
        {
            ret = this->new_node(i, std::move(sons));
            table.insert(ret, hash);
            this->for_each_son(ret, id_inc_ref_count<Data, Degree>);
        }

        // It is now safe to unmark them since they certainly have ref.
        this->for_each_son(ret, id_set_notmarked<Data, Degree>);
    }

    return id_set_marked(ret);
}

template<class Data, degree Degree, domain Domain>
auto node_manager<Data, Degree, Domain>::get_level(index_t const i) const
    -> level_t
{
    return indexToLevel_[i];
}

template<class Data, degree Degree, domain Domain>
auto node_manager<Data, Degree, Domain>::get_level(node_t* const n) const
    -> level_t
{
    return n->is_terminal() ? this->get_leaf_level()
                            : this->get_level(n->get_index());
}

template<class Data, degree Degree, domain Domain>
auto node_manager<Data, Degree, Domain>::get_leaf_level() const -> level_t
{
    return static_cast<level_t>(this->get_var_count());
}

template<class Data, degree Degree, domain Domain>
auto node_manager<Data, Degree, Domain>::get_index(level_t const l) const
    -> index_t
{
    assert(l < levelToIndex_.size());
    return levelToIndex_[l];
}

template<class Data, degree Degree, domain Domain>
auto node_manager<Data, Degree, Domain>::get_domain(index_t const i) const
    -> uint_t
{
    assert(i < this->get_var_count());
    return domains_[i];
}

template<class Data, degree Degree, domain Domain>
auto node_manager<Data, Degree, Domain>::get_node_count(index_t const i) const
    -> std::size_t
{
    assert(i < this->get_var_count());
    return uniqueTables_[i].size();
}

template<class Data, degree Degree, domain Domain>
auto node_manager<Data, Degree, Domain>::get_node_count(node_t* const n) const
    -> std::size_t
{
    auto count = 0ul;
    this->traverse_pre(
        n,
        [&count](auto const)
        {
            ++count;
        }
    );
    return count;
}

template<class Data, degree Degree, domain Domain>
auto node_manager<Data, Degree, Domain>::get_node_count() const -> std::size_t
{
    return nodeCount_;
}

template<class Data, degree Degree, domain Domain>
auto node_manager<Data, Degree, Domain>::get_var_count() const -> std::size_t
{
    return uniqueTables_.size();
}

template<class Data, degree Degree, domain Domain>
auto node_manager<Data, Degree, Domain>::get_order() const
    -> std::vector<index_t> const&
{
    return levelToIndex_;
}

template<class Data, degree Degree, domain Domain>
auto node_manager<Data, Degree, Domain>::get_domains() const
    -> std::vector<uint_t>
{
    auto ds = std::vector<uint_t>();
    for (auto k = 0u; k < this->get_var_count(); ++k)
    {
        ds.emplace_back(domains_[k]);
    }
    return ds;
}

template<class Data, degree Degree, domain Domain>
auto node_manager<Data, Degree, Domain>::force_gc() -> void
{
    this->collect_garbage();
    opCache_.rm_unused();
}

template<class Data, degree Degree, domain Domain>
auto node_manager<Data, Degree, Domain>::collect_garbage() -> void
{
    debug::out("node_manager: Collecting garbage. ");
    [[maybe_unused]] auto const before = nodeCount_;

    for (auto level = 0u; level < this->get_var_count(); ++level)
    {
        auto& table    = uniqueTables_[levelToIndex_[level]];
        auto const end = std::end(table);
        auto it        = std::begin(table);

        while (it != end)
        {
            auto const n = *it;
            if (can_be_gced(n))
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

    for (auto& t : terminals_)
    {
        if (t && can_be_gced(t))
        {
            this->delete_node(t);
            t = nullptr;
        }
    }

    for (auto& s : specials_)
    {
        if (s && can_be_gced(s))
        {
            this->delete_node(s);
            s = nullptr;
        }
    }

    debug::out(
        before - nodeCount_, " nodes collected.", " Now there are ", nodeCount_,
        " unique nodes.\n"
    );
}

template<class Data, degree Degree, domain Domain>
auto node_manager<Data, Degree, Domain>::to_dot_graph(std::ostream& ost) const
    -> void
{
    this->to_dot_graph_common(
        ost,
        [this](auto const& f)
        {
            this->for_each_node(f);
        }
    );
}

template<class Data, degree Degree, domain Domain>
auto node_manager<Data, Degree, Domain>::to_dot_graph(
    std::ostream& ost, node_t* const n
) const -> void
{
    this->to_dot_graph_common(
        ost,
        [this, n](auto const& f)
        {
            this->traverse_pre(n, f);
        }
    );
}

template<class Data, degree Degree, domain Domain>
auto node_manager<Data, Degree, Domain>::domain_product(
    level_t const from, level_t const to
) const -> std::size_t
{
    if constexpr (domains::is_fixed<Domain>()())
    {
        return utils::int_pow(Domain()(), to - from);
    }
    else
    {
        auto product = 1ul;
        for (auto l = from; l < to; ++l)
        {
            product *= domains_[levelToIndex_[l]];
        }
        return product;
    }
}

template<class Data, degree Degree, domain Domain>
template<utils::i_gen F>
auto node_manager<Data, Degree, Domain>::make_sons(index_t const i, F&& f)
    -> sons_t
{
    auto ss = node_t::container(domains_[i], Degree());
    for (auto k = uint_t {0}; k < domains_[i]; ++k)
    {
        ss[k] = std::invoke(f, k);
    }
    return ss;
}

template<class Data, degree Degree, domain Domain>
template<node_op<node<Data, Degree>> NodeOp>
auto node_manager<Data, Degree, Domain>::for_each_son(
    node_t* const node, NodeOp&& f
) const -> void
{
    auto const i = node->get_index();
    for (auto k = 0u; k < domains_[i]; ++k)
    {
        std::invoke(f, node->get_son(k));
    }
}

template<class Data, degree Degree, domain Domain>
template<node_op<node<Data, Degree>> NodeOp>
auto node_manager<Data, Degree, Domain>::for_each_son(
    index_t const i, sons_t const& sons, NodeOp&& f
) const -> void
{
    for (auto k = 0u; k < domains_[i]; ++k)
    {
        std::invoke(f, sons[k]);
    }
}

template<class Data, degree Degree, domain Domain>
template<node_op<node<Data, Degree>> NodeOp>
auto node_manager<Data, Degree, Domain>::for_each_node(NodeOp&& f) const -> void
{
    for (auto const& table : uniqueTables_)
    {
        for (auto const n : table)
        {
            std::invoke(f, n);
        }
    }

    this->for_each_terminal_node(f);
}

template<class Data, degree Degree, domain Domain>
template<node_op<node<Data, Degree>> NodeOp>
auto node_manager<Data, Degree, Domain>::for_each_terminal_node(NodeOp&& f
) const -> void
{
    for (auto const n : terminals_)
    {
        if (n)
        {
            std::invoke(f, n);
        }
    }
}

template<class Data, degree Degree, domain Domain>
template<bin_op O>
auto node_manager<Data, Degree, Domain>::cache_find(
    node_t* const l, node_t* const r
) -> node_t*
{
    auto const node = opCache_.template find<O>(l, r);
    if (node)
    {
        id_set_marked(node);
    }
    return node;
}

template<class Data, degree Degree, domain Domain>
template<bin_op O>
auto node_manager<Data, Degree, Domain>::cache_put(
    node_t* const l, node_t* const r, node_t* const res
) -> void
{
    opCache_.template put<O>(l, r, res);
}

template<class Data, degree Degree, domain Domain>
auto node_manager<Data, Degree, Domain>::is_valid_var_value(
    index_t const i, uint_t const v
) const -> bool
{
    return v < domains_[i];
}

template<class Data, degree Degree, domain Domain>
auto node_manager<Data, Degree, Domain>::run_deferred() -> void
{
    if (gcReorderDeferred_)
    {
        this->collect_garbage();
        opCache_.clear();
        this->sift_variables();
    }
}

template<class Data, degree Degree, domain Domain>
template<node_op<node<Data, Degree>> NodeOp>
auto node_manager<Data, Degree, Domain>::traverse_pre(
    node_t* const node, NodeOp&& nodeOp
) const -> void
{
    auto const go =
        [this](auto const& self, auto const n, auto const& op) -> void
    {
        n->toggle_marked();
        std::invoke(op, n);
        if (n->is_internal())
        {
            this->for_each_son(
                n,
                [&self, n, &op](auto const son) -> void
                {
                    if (n->is_marked() != son->is_marked())
                    {
                        self(self, son, op);
                    }
                }
            );
        }
    };

    go(go, node, nodeOp);
    this->traverse_no_op(node); // Second traverse to reset marks.
}

template<class Data, degree Degree, domain Domain>
template<node_op<node<Data, Degree>> NodeOp>
auto node_manager<Data, Degree, Domain>::traverse_post(
    node_t* const node, NodeOp&& nodeOp
) const -> void
{
    auto const go = [this](auto& self, auto const n, auto& op) -> void
    {
        n->toggle_marked();
        if (n->is_internal())
        {
            this->for_each_son(
                n,
                [&self, n, &op](auto const son)
                {
                    if (n->is_marked() != son->is_marked())
                    {
                        self(self, son, op);
                    }
                }
            );
        }
        std::invoke(op, n);
    };

    go(go, node, nodeOp);
    this->traverse_no_op(node); // Second traverse to reset marks.
}

template<class Data, degree Degree, domain Domain>
template<node_op<node<Data, Degree>> NodeOp>
auto node_manager<Data, Degree, Domain>::traverse_level(
    node_t* const node, NodeOp&& nodeOp
) const -> void
{
    auto const cmp = [this](auto const l, auto const r)
    {
        return this->get_level(l) > this->get_level(r);
    };

    using compare_t = decltype(cmp);
    using node_prio_q =
        std::priority_queue<node_t*, std::vector<node_t*>, compare_t>;
    auto queue = node_prio_q(cmp);
    node->toggle_marked();
    queue.push(node);
    while (not queue.empty())
    {
        auto const current = queue.top();
        queue.pop();
        std::invoke(nodeOp, current);
        if (current->is_internal())
        {
            this->for_each_son(
                current,
                [&queue, current](auto const son)
                {
                    if (son->is_marked() != current->is_marked())
                    {
                        queue.push(son);
                        son->toggle_marked();
                    }
                }
            );
        }
    }

    this->traverse_no_op(node); // Second traverse to reset marks.
}

template<class Data, degree Degree, domain Domain>
auto node_manager<Data, Degree, Domain>::dec_ref_count(node_t* const v) -> void
{
    v->dec_ref_count();
}

template<class Data, degree Degree, domain Domain>
auto node_manager<Data, Degree, Domain>::is_redundant(
    index_t const i, sons_t const& sons
) const -> bool
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
auto node_manager<Data, Degree, Domain>::adjust_tables() -> void
{
    debug::out(
        "node_manager: Adjusting unique tables.", " Node count is ", nodeCount_,
        ".\n"
    );

    for (auto i = 0u; i < uniqueTables_.size(); ++i)
    {
        uniqueTables_[i].adjust_capacity(domains_[i]);
    }
}

template<class Data, degree Degree, domain Domain>
auto node_manager<Data, Degree, Domain>::adjust_caches() -> void
{
    auto const newSize = cacheRatio_ * static_cast<double>(nodeCount_);
    opCache_.adjust_capacity(static_cast<std::size_t>(newSize));
}

template<class Data, degree Degree, domain Domain>
template<class... Args>
auto node_manager<Data, Degree, Domain>::new_node(Args&&... args) -> node_t*
{
    if (autoReorderEnabled_)
    {
        // GC + reorder will be done after current
        // high level operations finishes.
        // Until then, just create a new pool.

        if (pool_.available_node_count() == 0)
        {
            pool_.grow();
            this->deferr_gc_reorder();
        }
    }
    else
    {
        // Run GC. If not enough nodes are collected,
        // preventively create a new pool.

        if (pool_.available_node_count() == 0)
        {
            auto const growThreshold = static_cast<std::size_t>(
                gcRatio_ * static_cast<double>(pool_.main_pool_size())
            );

            this->force_gc();

            if (pool_.available_node_count() < growThreshold)
            {
                pool_.grow();
            }
        }
    }

    if (nodeCount_ >= nextTableAdjustment_)
    {
        // When the number of nodes doubles,
        // adjust cache and table sizes.
        this->adjust_tables();
        this->adjust_caches();
        nextTableAdjustment_ *= 2;
    }

    ++nodeCount_;
    return pool_.create(std::forward<Args>(args)...);
}

template<class Data, degree Degree, domain Domain>
auto node_manager<Data, Degree, Domain>::delete_node(node_t* const n) -> void
{
    assert(not n->is_marked());
    --nodeCount_;
    n->set_unused();
    pool_.destroy(n);
}

template<class Data, degree Degree, domain Domain>
auto node_manager<Data, Degree, Domain>::traverse_no_op(node_t* const node
) const -> void
{
    auto const go = [this](auto self, auto const n) -> void
    {
        n->toggle_marked();
        if (n->is_internal())
        {
            this->for_each_son(
                n,
                [self, n](auto const son) -> void
                {
                    if (n->is_marked() != son->is_marked())
                    {
                        self(self, son);
                    }
                }
            );
        }
    };
    go(go, node);
}

template<class Data, degree Degree, domain Domain>
template<class ForEachNode>
auto node_manager<Data, Degree, Domain>::to_dot_graph_common(
    std::ostream& ost, ForEachNode&& for_each_node
) const -> void
{
    auto const make_label = [](auto const n)
    {
        if (n->is_terminal())
        {
            using namespace std::literals::string_literals;
            auto const val = n->get_value();
            return val == Undefined ? "*"s : std::to_string(val);
        }
        else
        {
            return "x" + std::to_string(n->get_index());
        }
    };

    auto const get_id_str = [](auto const n)
    {
        return std::to_string(reinterpret_cast<std::uintmax_t>(n));
    };

    auto const output_range = [](auto& ostr, auto const& xs, auto const sep)
    {
        auto const end = std::end(xs);
        auto it        = std::begin(xs);
        while (it != end)
        {
            ostr << *it;
            ++it;
            if (it != end)
            {
                ostr << sep;
            }
        }
    };

    auto const levelCount = 1 + this->get_var_count();
    auto labels           = std::vector<std::string>();
    auto rankGroups       = std::vector<std::vector<std::string>>(levelCount);
    auto arcs             = std::vector<std::string>();
    auto squareShapes     = std::vector<std::string>();

    for_each_node(
        [&, this](auto const n)
        {
            // Create label.
            auto const level = this->get_level(n);
            labels.emplace_back(
                get_id_str(n) + R"( [label = ")" + make_label(n) +
                R"(", tooltip = ")" + std::to_string(n->get_ref_count()) +
                R"("];)"
            );

            if (n->is_terminal())
            {
                squareShapes.emplace_back(get_id_str(n));
                rankGroups.back().emplace_back(get_id_str(n) + ";");
                return;
            }

            // Add to same level.
            rankGroups[level].emplace_back(get_id_str(n) + ";");

            // Add arcs.
            this->for_each_son(
                n,
                [&, k = 0](auto const son) mutable
                {
                    if constexpr (std::is_same_v<Degree, degrees::fixed<2>>)
                    {
                        arcs.emplace_back(
                            get_id_str(n) + " -> " + get_id_str(son) +
                            " [style = " + (0 == k ? "dashed" : "solid") + "];"
                        );
                    }
                    else
                    {
                        arcs.emplace_back(
                            get_id_str(n) + " -> " + get_id_str(son) +
                            R"( [label = )" + std::to_string(k) + "];"
                        );
                    }
                    ++k;
                }
            );
        }
    );

    // Finally, output everything into the output stream.
    ost << "digraph DD {" << '\n';
    ost << "    node [shape = square] ";
    output_range(ost, squareShapes, " ");
    ost << ";\n";
    ost << "    node [shape = circle];"
        << "\n\n";

    ost << "    ";
    output_range(ost, labels, "\n    ");
    ost << "\n\n";
    ost << "    ";
    output_range(ost, arcs, "\n    ");
    ost << "\n\n";

    for (auto const& rs : rankGroups)
    {
        if (not rs.empty())
        {
            ost << "    { rank = same; ";
            output_range(ost, rs, " ");
            ost << " }" << '\n';
        }
    }
    ost << '\n';
    ost << "}" << '\n';
}

template<class Data, degree Degree, domain Domain>
auto node_manager<Data, Degree, Domain>::deferr_gc_reorder() -> void
{
    gcReorderDeferred_ = true;
}

template<class Data, degree Degree, domain Domain>
auto node_manager<Data, Degree, Domain>::check_distinct(
    std::vector<index_t> const& is
) -> bool
{
    if (is.empty())
    {
        return true;
    }
    auto const me = *std::max_element(std::begin(is), std::end(is));
    auto in       = std::vector<bool>(me + 1, false);
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
auto node_manager<Data, Degree, Domain>::can_be_gced(node_t* const n) -> bool
{
    return n->get_ref_count() == 0 && not n->is_marked();
}

template<class Data, degree Degree, domain Domain>
auto node_manager<Data, Degree, Domain>::swap_node_with_next(node_t* const node)
    -> void
{
    auto const mkmatrix = [](auto const nRow, auto const nCol)
    {
        if constexpr (degrees::is_fixed<Degree>()())
        {
            auto constexpr N = Degree()();
            return std::array<std::array<node_t*, N>, N> {};
        }
        else
        {
            auto const row = std::vector<node_t*>(nCol, nullptr);
            return std::vector<std::vector<node_t*>>(nRow, row);
        }
    };

    auto const nodeIndex  = node->get_index();
    auto const nextIndex  = this->get_index(1 + this->get_level(node));
    auto const nodeDomain = this->get_domain(nodeIndex);
    auto const sonDomain  = this->get_domain(nextIndex);
    auto const oldSons    = this->make_sons(
        nodeIndex,
        [node](auto const k)
        {
            return node->get_son(k);
        }
    );

    auto cofactorMatrix = mkmatrix(nodeDomain, sonDomain);
    for (auto nk = 0u; nk < nodeDomain; ++nk)
    {
        auto const son = node->get_son(nk);
        for (auto sk = 0u; sk < sonDomain; ++sk)
        {
            auto const justUseSon =
                son->is_terminal() || son->get_index() != nextIndex;
            cofactorMatrix[nk][sk] = justUseSon ? son : son->get_son(sk);
        }
    }

    node->set_index(nextIndex);
    auto newSons = this->make_sons(
        nextIndex,
        [&, this](auto const nk)
        {
            return this->internal_node(
                nodeIndex, this->make_sons(
                               nodeIndex,
                               [&](auto const sk)
                               {
                                   return cofactorMatrix[sk][nk];
                               }
                           )
            );
        }
    );
    node->set_sons(std::move(newSons));
    this->for_each_son(node, id_inc_ref_count<Data, Degree>);
    this->for_each_son(node, id_set_notmarked<Data, Degree>);
    this->for_each_son(
        nodeIndex, oldSons,
        [this](auto const os)
        {
            this->dec_ref_try_gc(os);
        }
    );
}

template<class Data, degree Degree, domain Domain>
auto node_manager<Data, Degree, Domain>::dec_ref_try_gc(node_t* const n) -> void
{
    n->dec_ref_count();

    if (not can_be_gced(n))
    {
        return;
    }

    if (n->is_internal())
    {
        this->for_each_son(
            n,
            [this](auto const s)
            {
                this->dec_ref_try_gc(s);
            }
        );

        uniqueTables_[n->get_index()].erase(n, domains_[n->get_index()]);
    }
    else
    {
        if (is_special(n->get_value()))
        {
            specials_[special_val_to_index(n->get_value())] = nullptr;
        }
        else
        {
            terminals_[n->get_value()] = nullptr;
        }
    }

    this->delete_node(n);
}

template<class Data, degree Degree, domain Domain>
auto node_manager<Data, Degree, Domain>::swap_variable_with_next(
    index_t const index
) -> void
{
    auto const level     = this->get_level(index);
    auto const nextIndex = this->get_index(1 + level);
    auto tmpTable        = unique_table_t(std::move(uniqueTables_[index]));
    for (auto const n : tmpTable)
    {
        this->swap_node_with_next(n);
    }
    uniqueTables_[index].adjust_capacity(domains_[index]);
    uniqueTables_[nextIndex].merge(std::move(tmpTable), domains_[nextIndex]);

    using std::swap;
    swap(levelToIndex_[level], levelToIndex_[1 + level]);
    ++indexToLevel_[index];
    --indexToLevel_[nextIndex];
}

template<class Data, degree Degree, domain Domain>
auto node_manager<Data, Degree, Domain>::sift_variables() -> void
{
    using count_pair = struct
    {
        index_t index_;
        std::size_t count_;
    };

    // Sorts indices by number of nodes with given index descending.
    auto const determine_sift_order = [this]()
    {
        auto const varCount = this->get_var_count();
        auto counts         = utils::fill_vector(
            varCount,
            [this](auto const i)
            {
                return count_pair {i, this->get_node_count(i)};
            }
        );
        std::sort(
            std::begin(counts), std::end(counts),
            [](auto const& l, auto const& r)
            {
                return l.count_ > r.count_;
            }
        );
        return counts;
    };

    // Moves variable one level down.
    auto const move_var_down = [this](auto const index)
    {
        this->swap_variable_with_next(index);
    };

    // Moves variable one level up.
    auto const move_var_up = [this](auto const index)
    {
        auto const level     = this->get_level(index);
        auto const prevIndex = this->get_index(level - 1);
        this->swap_variable_with_next(prevIndex);
    };

    // Tries to place variable on each level.
    // In the end, restores position with lowest total number of nodes.
    auto const place_variable = [&, this](auto const index)
    {
        auto const lastInternalLevel = this->get_var_count() - 1;
        auto currentLevel            = this->get_level(index);
        auto optimalLevel            = currentLevel;
        auto optimalCount            = nodeCount_;

        // Sift down.
        while (currentLevel != lastInternalLevel)
        {
            move_var_down(index);
            ++currentLevel;
            if (nodeCount_ < optimalCount)
            {
                optimalCount = nodeCount_;
                optimalLevel = currentLevel;
            }
        }

        // Sift up.
        while (currentLevel != 0)
        {
            move_var_up(index);
            --currentLevel;
            if (nodeCount_ < optimalCount)
            {
                optimalCount = nodeCount_;
                optimalLevel = currentLevel;
            }
        }

        // Restore optimal position.
        while (currentLevel != optimalLevel)
        {
            move_var_down(index);
            ++currentLevel;
        }
    };

    debug::out(
        "node_manager: Sifting variables. Node count before ", nodeCount_, ".\n"
    );

    auto const siftOrder = determine_sift_order();
    for (auto const pair : siftOrder)
    {
        place_variable(pair.index_);
    }

    debug::out(
        "node_manager: Done sifting. Node count after ", nodeCount_, ".\n"
    );

    gcReorderDeferred_ = false;
}
} // namespace teddy

#endif