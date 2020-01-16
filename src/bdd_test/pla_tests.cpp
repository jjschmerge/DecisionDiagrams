#include "pla_tests.hpp"

#include "pla_file.hpp"
#include "diagram_tests.hpp"
#include "../bdd/bdd_pla.hpp"
#include "../bdd/pla_function.hpp"
#include "../utils/io.hpp"

namespace mix::dd
{
    auto test_pla_creator (const pla_file& file) -> bool
    {
        bdds_from_pla<empty, empty> plaCreator;
        
        const auto diagrams      {plaCreator.create(file)};
        const auto functionCount {file.function_count()};
        auto       plaFunctions  {pla_function::create_from_file(file)};

        for (int32_t i {0}; i < functionCount; ++i)
        {
            utils::print(std::to_string(i) + ". ");
            
            const auto testResult 
            {
                random_test_diagram(plaFunctions.at(i), diagrams.at(i))
            };

            if (! testResult)
            {
                utils::printl("Error in diagram with index " + std::to_string(i));
                return false;
            }
        }

        utils::printl("All diagrams are correct.");
        return true;
    }
}