#ifndef MIX_DD_MDD_MANAGER_HPP
#define MIX_DD_MDD_MANAGER_HPP

#include "diagrams/mdd.hpp"
#include "diagrams/vertex_manager.hpp"
#include "diagrams/var_vals.hpp"
#include "diagrams/operators.hpp"
#include <array>

namespace mix::dd
{
    template<class VertexData, class ArcData, std::size_t P>
    class mdd_manager
    {
    /* Public aliases */
    public:
        using mdd_t      = mdd<VertexData, ArcData, P>;
        using mdd_v      = std::vector<mdd_t>;
        using vertex_t   = vertex<VertexData, ArcData, P>;
        using log_t      = typename log_val_traits<P>::type;
        using log_v      = std::vector<log_t>;
        using index_v    = std::vector<index_t>;
        using level_v    = std::vector<level_t>;
        using prob_table = std::vector<std::array<double, P>>;
        using double_v   = std::vector<double>;

    /* Constructors & other */
    public:
        mdd_manager (std::size_t const varCount);
        mdd_manager (mdd_manager const&) = delete;

        auto set_domains     (log_v domains)        -> void;
        auto set_order       (index_v levelToIndex) -> void;
        auto swap_vars       (index_t const i)      -> void;
        auto clear           () -> void;
        auto clear_cache     () -> void;
        auto collect_garbage () -> void;

    /* Tools */
    public:
        auto vertex_count   ()                  const -> std::size_t;
        auto vertex_count   (mdd_t const& d)    const -> std::size_t;
        auto vertex_count   (index_t const i)   const -> std::size_t;
        auto to_dot_graph   (std::ostream& ost) const -> void;
        auto to_dot_graph   (std::ostream& ost, mdd_t const& d) const -> void;
        auto satisfy_count  (log_t const val, mdd_t& d) -> std::size_t;
        auto dependency_set (mdd_t const& d) -> std::vector<index_t>;

        template< class VariableValues
                , class GetIthVal = get_var_val<P, VariableValues> >
        auto evaluate (mdd_t const& d, VariableValues const& vs) const -> log_t;

        // TODO rename to satisfy_all_g (_g for generic) and add simler version that returns vector
        template< class VariableValues
                , class OutputIt
                , class SetVarVal = set_var_val<P, VariableValues> >
        auto satisfy_all (log_t const val, mdd_t const& d, OutputIt out) const -> void; 

        // TODO traversy do private
        template<class VertexOp>
        auto traverse_pre (mdd_t const& d, VertexOp&& op) const -> void;

        template<class VertexOp>
        auto traverse_post (mdd_t const& d, VertexOp&& op) const -> void;

        template<class VertexOp>
        auto traverse_level (mdd_t const& d, VertexOp&& op) const -> void;

    /* Manipulation */
    public:
        template<template<std::size_t> class Op>
        auto apply (mdd_t const& lhs, mdd_t const& rhs) -> mdd_t;

        auto restrict_var (mdd_t const& d, index_t const i, log_t const val) -> mdd_t;

        template<template<std::size_t> class Op>
        auto left_fold (mdd_v const& ds) -> mdd_t;

        template<template<std::size_t> class Op>
        auto tree_fold (mdd_v& ds) -> mdd_t;

        template<template<std::size_t> class Op, class InputIt>
        auto left_fold (InputIt first, InputIt last) -> mdd_t;

        template<template<std::size_t> class Op, class RandomIt>
        auto tree_fold (RandomIt first, RandomIt last) -> mdd_t;

    /* Creation */
    public:
        auto constant   (log_t const val)   -> mdd_t;
        auto variable   (index_t const i)   -> mdd_t;
        auto variables  (index_v const& is) -> mdd_v;
        auto operator() (index_t const i)   -> mdd_t;

    /* Reliability */
    public:
        auto calculate_probabilities (prob_table const& ps, mdd_t& f)                    -> void;
        auto get_probability         (log_t const level) const                           -> double;
        auto get_availability        (log_t const level) const                           -> double;
        auto get_unavailability      (log_t const level) const                           -> double;
        auto availability            (log_t const level, prob_table const& ps, mdd_t& f) -> double;
        auto unavailability          (log_t const level, prob_table const& ps, mdd_t& f) -> double;

        auto dpbd              (val_change<P> const var, val_change<P> const f, mdd_t const& sf, index_t const i) -> mdd_t;
        auto dpbd_integrated_1 (val_change<P> const var, log_t const fVal, mdd_t const& sf, index_t const i)      -> mdd_t;
        auto dpbd_integrated_2 (val_change<P> const var, mdd_t const& sf, index_t const i)                        -> mdd_t;
        auto dpbd_integrated_3 (val_change<P> const var, log_t const fVal, mdd_t const& sf, index_t const i)      -> mdd_t;

        auto dpbds              (val_change<P> const var, val_change<P> const f, mdd_t const& sf) -> mdd_v;
        auto dpbds_integrated_1 (val_change<P> const var, log_t const fVal, mdd_t const& sf)      -> mdd_v;
        auto dpbds_integrated_2 (val_change<P> const var, mdd_t const& sf)                        -> mdd_v;
        auto dpbds_integrated_3 (val_change<P> const var, log_t const fVal, mdd_t const& sf)      -> mdd_v;

        auto structural_importance  (mdd_t& dpbd, index_t const i) -> double;
        auto structural_importances (mdd_v& dpbds)                 -> double_v;

        auto birnbaum_importance  (prob_table const& ps, mdd_t& dpbd)  -> double;
        auto birnbaum_importances (prob_table const& ps, mdd_v& dpbds) -> double_v;

        auto fussell_vesely_importance  (prob_table const& ps, double const U, mdd_t const& dpbd)  -> double;
        auto fussell_vesely_importances (prob_table const& ps, double const U, mdd_v const& dpbds) -> double_v;

        template<class VectorType>
        auto mcvs (mdd_v const& dpbds, log_t const level) -> std::vector<VectorType>;

    /* Internal aliases */
    protected:
        using vertex_a            = std::array<vertex_t*, P>;
        using vertex_v         = std::vector<vertex_t*>;
        using vertex_vv        = std::vector<vertex_v>;
        using apply_key_t      = std::tuple<vertex_t*, op_id_t, vertex_t*>;
        using apply_memo_t     = std::unordered_map<apply_key_t, vertex_t*, utils::tuple_hash_t<apply_key_t>>;
        using transform_key_t  = vertex_t*;
        using transform_memo_t = std::unordered_map<vertex_t*, vertex_t*>;
        using vertex_manager_t = vertex_manager<VertexData, ArcData, P>;

    /* Tools internals */
    private:
        template<class VertexIterator>
        auto to_dot_graph_impl (std::ostream& ost, VertexIterator for_each_v) const -> void;

        auto domain_product (level_t const from, level_t const to) const -> std::size_t;

        auto fill_levels (mdd_t const& d) const -> vertex_vv;

        template< class VariableValues
                , class OutputIt
                , class SetVarVal = set_var_val<P, VariableValues> >
        auto satisfy_all_step ( log_t const     val
                              , level_t const   l
                              , vertex_t* const v
                              , VariableValues& xs
                              , OutputIt&       out ) const -> void;

        template<class VertexOp>
        auto traverse_pre_step (vertex_t* const v, VertexOp&& op) const -> void;

        template<class VertexOp>
        auto traverse_post_step (vertex_t* const v, VertexOp&& op) const -> void;

    /* Manipulation internals */
    protected:
        template<template<std::size_t> class Op>
        auto apply_step (vertex_t* const lhs, vertex_t* const rhs) -> vertex_t*;

        template<class Op>
        static auto make_apply_key (vertex_t* const lhs, vertex_t* const rhs) -> apply_key_t;

        template<class Transformator>
        auto transform (mdd_t const& d, Transformator&& transform_sons) -> mdd_t;

        template<class Transformator>
        auto transform_step (vertex_t* const v, Transformator&& transform_sons) -> vertex_t*;

    /* Creator internals */
    protected:
        auto variable   (index_t const i, log_t const domain) -> mdd_t;
        auto operator() (index_t const i, log_t const domain) -> mdd_t;

        template<class LeafVals>
        auto variable_impl (index_t const i, LeafVals&& vals, std::size_t const domain = P) -> mdd_t;

    /* Reliability internals */
        public:
        auto sum_terminals         (log_t const from, log_t const to) const -> double;
        auto structural_importance (std::size_t const domainSize, mdd_t& dpbd) -> double;
        auto to_dpbde              (mdd_t const& dpbd, log_t const level, index_t const i) -> mdd_t;
        auto to_mnf                (mdd_t const& dpbd) -> mdd_t;

    /* Other internals */
    public:
        auto var_count          () const -> std::size_t;
        auto get_domain         (index_t const i) const -> log_t;
        auto get_domain_product () const -> std::size_t;

    /* Member variables */
    protected:
        vertex_manager_t vertexManager_;
        apply_memo_t     applyMemo_;
        transform_memo_t transformMemo_;
        log_v            domains_;
    };

    template<std::size_t P>
    auto make_mdd_manager(std::size_t const varCount);

    template<class VertexData, class ArcData, std::size_t P>
    auto register_manager(mdd_manager<VertexData, ArcData, P>& m);

    template<class VertexData, class ArcData, std::size_t P>
    auto operator&& ( mdd<VertexData, ArcData, P> const& lhs
                    , mdd<VertexData, ArcData, P> const& rhs ) -> mdd<VertexData, ArcData, P>;

    template<class VertexData, class ArcData, std::size_t P>
    auto operator|| ( mdd<VertexData, ArcData, P> const& lhs
                    , mdd<VertexData, ArcData, P> const& rhs ) -> mdd<VertexData, ArcData, P>;

    template<class VertexData, class ArcData, std::size_t P>
    auto operator^ ( mdd<VertexData, ArcData, P> const& lhs
                   , mdd<VertexData, ArcData, P> const& rhs ) -> mdd<VertexData, ArcData, P>;

    template<class VertexData, class ArcData, std::size_t P>
    auto operator== ( mdd<VertexData, ArcData, P> const& lhs
                    , mdd<VertexData, ArcData, P> const& rhs ) -> mdd<VertexData, ArcData, P>;

    template<class VertexData, class ArcData, std::size_t P>
    auto operator!= ( mdd<VertexData, ArcData, P> const& lhs
                    , mdd<VertexData, ArcData, P> const& rhs ) -> mdd<VertexData, ArcData, P>;

    template<class VertexData, class ArcData, std::size_t P>
    auto operator< ( mdd<VertexData, ArcData, P> const& lhs
                   , mdd<VertexData, ArcData, P> const& rhs ) -> mdd<VertexData, ArcData, P>;

    template<class VertexData, class ArcData, std::size_t P>
    auto operator<= ( mdd<VertexData, ArcData, P> const& lhs
                    , mdd<VertexData, ArcData, P> const& rhs ) -> mdd<VertexData, ArcData, P>;

    template<class VertexData, class ArcData, std::size_t P>
    auto operator> ( mdd<VertexData, ArcData, P> const& lhs
                   , mdd<VertexData, ArcData, P> const& rhs ) -> mdd<VertexData, ArcData, P>;

    template<class VertexData, class ArcData, std::size_t P>
    auto operator>= ( mdd<VertexData, ArcData, P> const& lhs
                    , mdd<VertexData, ArcData, P> const& rhs ) -> mdd<VertexData, ArcData, P>;

    template<class VertexData, class ArcData, std::size_t P>
    auto operator+ ( mdd<VertexData, ArcData, P> const& lhs
                   , mdd<VertexData, ArcData, P> const& rhs ) -> mdd<VertexData, ArcData, P>;

    template<class VertexData, class ArcData, std::size_t P>
    auto operator* ( mdd<VertexData, ArcData, P> const& lhs
                   , mdd<VertexData, ArcData, P> const& rhs ) -> mdd<VertexData, ArcData, P>;

    template<class VertexData, class ArcData, std::size_t P>
    auto operator! ( mdd<VertexData, ArcData, P> const& lhs ) -> mdd<VertexData, ArcData, P>;
}

#include "diagrams/mdd_manager.ipp"
#include "diagrams/mdd_manager_tools.ipp"
#include "diagrams/mdd_manager_manipulator.ipp"
#include "diagrams/mdd_manager_creator.ipp"
#include "diagrams/mdd_manager_reliability.ipp"

#endif