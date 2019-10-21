#include "bin_decision_diagram.hpp"

#include <sstream>
#include <set>
#include <queue>
#include <bitset>

namespace mix::dd
{
    bin_decision_diagram::bin_decision_diagram(vertex* pRoot
                      , std::map<log_val_t, vertex*>&& pValToLeaf
                      , std::map<vertex*, log_val_t>&& pLeafToVal) :
        root {pRoot}
      , valToLeaf {std::move(pValToLeaf)}
      , leafToVal {std::move(pLeafToVal)}
    {
    }

    bin_decision_diagram::~bin_decision_diagram()
    {
        // TODO foreach metodu
        std::set<vertex*> processed;
        std::queue<vertex*> toProcess;
        toProcess.push(this->root);

        while (! toProcess.empty())
        {
            vertex* v {toProcess.front()};
            toProcess.pop();

            if (processed.find(v) != processed.end())
            {
                continue;
            }

            for (arc& a : v->forwardStar)
            {
                if (a.target)
                {
                    toProcess.push(a.target);
                }
            }
            
            processed.insert(v);
        }

        for (vertex* v : processed)
        {
            delete v;
        }        
    }

    auto bin_decision_diagram::to_dot_graph () const -> std::string
    {
        std::ostringstream ost;

        std::set<vertex*> processed;
        std::queue<vertex*> toProcess;
        toProcess.push(this->root);

        ost << "digraph D {" << '\n'
            << "    node [shape = square] 0 1;" << '\n'
            << "    node [shape = circle];"     << "\n\n";

        while (! toProcess.empty())
        {
            vertex* v {toProcess.front()};
            toProcess.pop();

            if (v->is_leaf() || processed.find(v) != processed.end())
            {
                continue;
            }
            
            vertex* negativeTarget {v->forwardStar[0].target};
            vertex* positiveTarget {v->forwardStar[1].target};

            ost << "    " << v->label << " -> " << negativeTarget->label << " [style = dashed];" << '\n';
            ost << "    " << v->label << " -> " << positiveTarget->label << " [style = solid];"  << '\n';

            processed.insert(v);
            toProcess.push(negativeTarget);
            toProcess.push(positiveTarget);
        }

        ost << '}' << '\n';

        return ost.str();
    }

    auto bin_decision_diagram::get_value (const input_t input) const -> log_val_t
    {
        const std::bitset<sizeof(input_t)> inputBitSet {input};

        vertex* currentVertex {this->root};

        while (! currentVertex->is_leaf())
        {
            // TODO nejako treba v diagrame vediet pocet premenných
            // casom asi aj vsetky premenne...
            // zatial len napevno 3
            const size_t    bitIndex {3 - currentVertex->level - 1};
            const log_val_t variableValue {inputBitSet[bitIndex]};
            currentVertex = currentVertex->forwardStar[variableValue].target;
        }
        
        return this->leafToVal.at(currentVertex);
    }
}