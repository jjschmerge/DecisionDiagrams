#include <iostream>
#include <librog/rog.hpp>
#include <libteddy/teddy.hpp>

#include "common_test_setup.hpp"
#include "table_reliability.hpp"

namespace teddy
{

/**
 *  \brief Tests calculation of system state probabilities.
 */
template<class Settings>
class test_probability : public test_base<Settings>
{
public:
    test_probability(Settings settings)
        : test_base<Settings>("probability", std::move(settings))
    {
    }

protected:
    auto test() -> void override
    {
        auto expr     = make_expression(this->settings(), this->rng());
        auto manager  = make_manager(this->settings(), this->rng());
        auto diagram  = make_diagram(expr, manager);
        auto ps       = make_probabilities(manager, this->rng());
        auto domains  = manager.get_domains();
        auto table    = truth_table(make_vector(expr, domains), domains);
        auto const m  = std::ranges::max(manager.get_domains());
        auto expected = std::vector<double>(m);
        auto actual   = std::vector<double>(m);

        for (auto j = 0u; j < m; ++j)
        {
            expected[j] = probability(table, ps, j);
        }

        manager.calculate_probabilities(ps, diagram);
        for (auto j = 0u; j < m; ++j)
        {
            actual[j] = manager.get_probability(j);
        }

        for (auto j = 0u; j < m; ++j)
        {
            this->assert_equals(actual[j], expected[j], 0.00000001);
        }

        for (auto j = 0u; j < m; ++j)
        {
            actual[j] = manager.probability(j, ps, diagram);
        }

        for (auto j = 0u; j < m; ++j)
        {
            this->assert_equals(actual[j], expected[j], 0.00000001);
        }
    }
};

template<class Settings>
class test_availability : public test_base<Settings>
{
public:
    test_availability(Settings settings)
        : test_base<Settings>("availability", std::move(settings))
    {
    }

protected:
    auto test () -> void override
    {
        auto expr     = make_expression(this->settings(), this->rng());
        auto manager  = make_manager(this->settings(), this->rng());
        auto diagram  = make_diagram(expr, manager);
        auto ps       = make_probabilities(manager, this->rng());
        auto domains  = manager.get_domains();
        auto table    = truth_table(make_vector(expr, domains), domains);
        auto const m  = std::ranges::max(manager.get_domains());
        auto expected = std::vector<double>(m);
        auto actual   = std::vector<double>(m);

        for (auto j = 0u; j < m; ++j)
        {
            expected[j] = availability(table, ps, j);
        }

        for (auto j = 0u; j < m; ++j)
        {
            actual[j] = manager.availability(j, ps, diagram);
        }

        for (auto j = 0u; j < m; ++j)
        {
            this->assert_equals(expected[j], actual[j], 0.00000001);
        }

        manager.calculate_probabilities(ps, diagram);
        for (auto j = 0u; j < m; ++j)
        {
            actual[j] = manager.get_availability(j);
        }
    }
};

template<class Settings>
class test_unavailability : public test_base<Settings>
{
public:
    test_unavailability(Settings settings)
        : test_base<Settings>("unavailability", std::move(settings))
    {
    }

protected:
    auto test () -> void override
    {
        auto expr     = make_expression(this->settings(), this->rng());
        auto manager  = make_manager(this->settings(), this->rng());
        auto diagram  = make_diagram(expr, manager);
        auto ps       = make_probabilities(manager, this->rng());
        auto domains  = manager.get_domains();
        auto table    = truth_table(make_vector(expr, domains), domains);
        auto const m  = std::ranges::max(manager.get_domains());
        auto expected = std::vector<double>(m);
        auto actual   = std::vector<double>(m);

        for (auto j = 0u; j < m; ++j)
        {
            expected[j] = unavailability(table, ps, j);
        }

        for (auto j = 0u; j < m; ++j)
        {
            actual[j] = manager.unavailability(j, ps, diagram);
        }

        for (auto j = 0u; j < m; ++j)
        {
            this->assert_equals(expected[j], actual[j], 0.00000001);
        }

        manager.calculate_probabilities(ps, diagram);
        for (auto j = 0u; j < m; ++j)
        {
            actual[j] = manager.get_unavailability(j);
        }
    }
};

template<class Settings>
class test_state_frequency : public test_base<Settings>
{
public:
    test_state_frequency(Settings settings)
        : test_base<Settings>("state-frequency", std::move(settings))
    {
    }

protected:
    auto test () -> void override
    {
        auto expr     = make_expression(this->settings(), this->rng());
        auto manager  = make_manager(this->settings(), this->rng());
        auto diagram  = make_diagram(expr, manager);
        auto ps       = make_probabilities(manager, this->rng());
        auto domains  = manager.get_domains();
        auto table    = truth_table(make_vector(expr, domains), domains);
        auto const m  = std::ranges::max(manager.get_domains());
        auto expected = std::vector<double>(m);
        auto actual   = std::vector<double>(m);

        for (auto j = 0u; j < m; ++j)
        {
            expected[j] = state_frequency(table, j);
        }

        for (auto j = 0u; j < m; ++j)
        {
            actual[j] = manager.state_frequency(diagram, j);
        }

        for (auto j = 0u; j < m; ++j)
        {
            this->assert_equals(expected[j], actual[j], 0.00000001);
        }
    }
};

/**
 *  \brief Composite test for reliability manager.
 */
template<class ManagerSettings, class ExpressionSettings>
class test_reliability_manager : public rog::CompositeTest
{
public:
    test_reliability_manager(
        std::size_t const seed, ManagerSettings manager,
        ExpressionSettings expr, std::string name
    )
        : rog::CompositeTest(std::move(name))
    {
        auto seeder      = std::mt19937_64(seed);
        using settings_t = test_settings<ManagerSettings, ExpressionSettings>;

        this->add_test(std::make_unique<test_probability<settings_t>>(
            settings_t {seeder(), manager, expr}
        ));

        this->add_test(std::make_unique<test_availability<settings_t>>(
            settings_t {seeder(), manager, expr}
        ));

        this->add_test(std::make_unique<test_unavailability<settings_t>>(
            settings_t {seeder(), manager, expr}
        ));

        this->add_test(std::make_unique<test_state_frequency<settings_t>>(
            settings_t {seeder(), manager, expr}
        ));
    }
};

/**
 *  \brief Tests bss_manager.
 */
class test_bss_manager : public test_reliability_manager<
                             bss_manager_settings, expression_tree_settings>
{
public:
    test_bss_manager(std::size_t const seed)
        : test_reliability_manager<
              bss_manager_settings, expression_tree_settings>(
              seed, bss_manager_settings {21, 2'000, random_order_tag()},
              expression_tree_settings {}, "bss_manager"
          )
    {
    }
};

/**
 *  \brief Tests mss_manager.
 */
template<unsigned int M>
class test_mss_manager : public test_reliability_manager<
                             mss_manager_settings<M>,
                             expression_tree_settings
                         >
{
public:
    test_mss_manager(std::size_t const seed)
        : test_reliability_manager<
              mss_manager_settings<M>, expression_tree_settings>(
              seed,
              mss_manager_settings<M> {15, 5'000, random_order_tag()},
              expression_tree_settings {},
              "mss_manager"
          )
    {
    }
};

/**
 *  \brief Tests imss_manager.
 */
template<unsigned int M>
class test_imss_manager : public test_reliability_manager<
                             imss_manager_settings<M>,
                             expression_tree_settings
                          >
{
public:
    test_imss_manager(std::size_t const seed)
        : test_reliability_manager<
              imss_manager_settings<M>, expression_tree_settings>(
              seed,
              imss_manager_settings<M> {15, 5'000, random_order_tag(), random_domains()},
              expression_tree_settings {},
              "imss_manager"
          )
    {
    }
};

/**
 *  \brief Tests imss_manager.
 */
template<unsigned int M>
class test_ifmss_manager : public test_reliability_manager<
                             ifmss_manager_settings<M>,
                             expression_tree_settings
                           >
{
public:
    test_ifmss_manager(std::size_t const seed)
        : test_reliability_manager<
              ifmss_manager_settings<M>, expression_tree_settings>(
              seed,
              ifmss_manager_settings<M> {15, 5'000, random_order_tag(), random_domains()},
              expression_tree_settings {},
              "ifmss_manager"
          )
    {
    }
};

} // namespace teddy

auto run_test_one(std::size_t const seed)
{
    auto const M = 3;

    auto bssmt = teddy::test_bss_manager(seed);
    bssmt.run();
    rog::console_print_results(bssmt);

    auto mssmt = teddy::test_mss_manager<M>(seed);
    mssmt.run();
    rog::console_print_results(mssmt);

    auto imssmt = teddy::test_imss_manager<M>(seed);
    imssmt.run();
    rog::console_print_results(imssmt);

    auto ifmssmt = teddy::test_ifmss_manager<M>(seed);
    ifmssmt.run();
    rog::console_print_results(ifmssmt);
}

auto main(int const argc, char** const argv) -> int
{
    auto seed = 144ull;

    if (argc == 1)
    {
        std::cout << "./[bin] [one|many] {seed}" << '\n';
        return 1;
    }

    if (argc > 2)
    {
        auto const seedopt = teddy::utils::parse<std::size_t>(argv[2]);
        seed               = seedopt ? *seedopt : seed;
    }

    std::cout << "Seed is " << seed << '\n';

    if (argc > 1 && std::string_view(argv[1]) == "one")
    {
        run_test_one(seed);
    }
    else if (argc > 1 && std::string_view(argv[1]) == "many")
    {
        // run_test_many(seed);
    }
    else
    {
        // run_test_one(seed);
        // run_test_many(seed);
    }

    std::cout << '\n' << "End of main." << '\n';
    return 0;
}