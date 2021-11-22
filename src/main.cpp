#include "teddy/teddy.hpp"
#include <iostream>
#include <string>

using namespace teddy;

auto pla_sanity_check()
{
    using namespace std::string_literals;

    auto const plaDir = "/home/michal/Downloads/pla/"s;
    // auto const files = {"16-adder_col.pla", "15-adder_col.pla", "14-adder_col.pla", "13-adder_col.pla", "12-adder_col.pla", "11-adder_col.pla", "10-adder_col.pla"};
    // auto const files = {"14-adder_col.pla", "13-adder_col.pla", "12-adder_col.pla", "11-adder_col.pla", "10-adder_col.pla", "apex1.pla", "apex3_alt.pla", "apex5.pla", "seq.pla", "spla.pla"};
    auto const files = {"table3.pla"s};

    for (auto const& fileName : files)
    {
        auto const file = pla_file::load_file(plaDir + fileName);
        if (not file)
        {
            std::cout << "Failed to load '" << plaDir + fileName << "'.\n";
            continue;
        }
        auto manager    = bdd_manager((*file).variable_count(), 500);
        auto const ds   = manager.from_pla(*file, fold_type::Tree);
        auto sum        = 0ul;
        for (auto& d : ds)
        {
            sum += manager.node_count(d);
        }
        std::cout << fileName << " [" << sum << "] " << std::endl;
    }
}

auto main () -> int
{
    // class diagram_manager<Data, Degree, Domains>
    // class bin_diagram_manager<Data> : diagram_manager<Data, degrees::nary<2>,  domains::nary<2>>

    // class bdd_manager       : bin_diagram_manager<void>
        // TODO conditional_t pre P == 2 dediť z bin_diagram_manager
    // class mdd_manager<P>    : diagram_manager<void, degrees::nary<P>,  domains::nary<P>>
    // class imdd_manager      : diagram_manager<void, degrees::mixed,    domains::mixed>
    // class ifmdd_manager<PM> : diagram_manager<void, degrees::nary<PM>, domains::mixed>

        // TODO conditional_t pre P == 2 dediť z bin_diagram_manager
    // class hom_mss_reliability<P> : diagram_manager<double, degrees::nary<P>, domains::nary<P>>
    // class het_mss_reliability    : diagram_manager<double, degrees::mixed,   domains::mixed>
    // class bss_reliability        : hom_mss_reliability<2>

    // bdd_manager()
    // mdd_manager<P>()
    // imdd_manager(domains)
    // ifmdd_manager<P>(domains)

    // TODO bit flag in-use, set when vertex is moved back into pool,
    // vertices can be removed from cache based on this flag
    // unused vertices can be chained using the next member

    // pla_test_speed(1);
    // pla_sanity_check();
    // test_mdd_random<3>(10, order_e::Random, domain_e::Nonhomogenous);
    // test_mdd_vector(10);
    // test_bss();
    // test_mss();

    pla_sanity_check();

    std::cout << "Done." << '\n';
    return 0;
}