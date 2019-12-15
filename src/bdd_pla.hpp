#ifndef _BDD_PLA_
#define _BDD_PLA_

#include <string>
#include <vector>
#include <cmath>
#include <algorithm>
#include "typedefs.hpp"
#include "bdd.hpp"
#include "graph.hpp"

namespace mix::dd
{
    struct pla_line
    {
        // TODO použiť 2-bit set aby to bolo viac memmory friendly
        std::vector<log_val_t> varVals; 
        // TODO použiť 1-bit set aby to bolo viac memmory friendly
        std::vector<log_val_t> fVals;

        pla_line(const pla_line&) = delete;
        pla_line(pla_line&&) = default;
    };

    class pla_file
    {
    private:
        std::vector<pla_line> lines;

    public:
        pla_file(const pla_file&) = delete;
        pla_file(pla_file&&) = default;

        static auto read (const std::string& filePath) -> pla_file;

        auto variable_count () const -> int32_t;
        auto function_count () const -> int32_t;
        auto line_count     () const -> int32_t;
        auto get_lines      () const -> const std::vector<pla_line>&;

    private:
        static auto char_to_log_val (const char c) -> log_val_t;

        pla_file(std::vector<pla_line>&& pLines);
    };

    template<class VertexData = def_vertex_data_t
           , class ArcData = def_arc_data_t>
    class bdds_from_pla
    {
    public:
        using bdd_t  = bdd<VertexData, ArcData>;
        using vertex = typename graph<VertexData, ArcData>::vertex;
        using arc    = typename graph<VertexData, ArcData>::arc;

    public:
        auto create (const std::string& filePath) -> std::vector<bdd_t>;

    // TMP later private
    public: 
        auto create_diagram (const std::vector<log_val_t>& varVals
                           , const log_val_t fVal) -> bdd_t;
        
        auto merge_diagrams (std::vector<bdd_t> diagrams) -> bdd_t;
    };

    template<class VertexData, class ArcData>
    auto bdds_from_pla<VertexData, ArcData>::create 
        (const std::string& filePath) -> std::vector<bdd_t>
    {
        const pla_file file {pla_file::read(filePath)};

        std::vector< std::vector<bdd_t> > subDiagrams(file.function_count());

        const auto& plaLines {file.get_lines()};

        // https://stackoverflow.com/questions/13357065/how-does-openmp-handle-nested-loops
        // parallelizable for
        for (int32_t li {0}; li < file.line_count(); ++li)
        {
            // parallelizable for
            for (int32_t fi {0}; fi < file.function_count(); ++fi)
            {
                subDiagrams[fi].emplace_back(
                    this->create_diagram( plaLines[li].varVals, plaLines[li].fVals[fi] )
                );
            }
        }
        
        std::vector<bdd_t> finalDiagrams(file.function_count());
        // parallelizable for
        for (int32_t fi {0}; fi < file.function_count(); ++fi)
        {
            finalDiagrams[fi] = this->merge_diagrams(std::move(subDiagrams[fi]));
        }
        
        return finalDiagrams;
    }

    template<class VertexData, class ArcData>
    auto bdds_from_pla<VertexData, ArcData>::create_diagram
        (const std::vector<log_val_t>& varVals, const log_val_t fVal) -> bdd_t
    {
        const size_t leafLevel {varVals.size() + 1};
        size_t level  {1};
        id_t   nextId {1};

        std::vector<vertex*> relevantVariables;
        relevantVariables.reserve(varVals.size());

        for (auto val : varVals)
        {
            if (val != X)
            {
                relevantVariables.push_back(new vertex {nextId++, level});
            }

            ++level;
        }

        if (relevantVariables.empty())
        {
            throw std::runtime_error {"Invalid pla line."};
        }

        auto valLeaf {new vertex {nextId++, leafLevel}};
        auto xLeaf   {new vertex {nextId++, leafLevel}};

        auto b {relevantVariables.begin()};
        auto e {relevantVariables.end()};
        --e;

        while (b != e)
        {
            const auto v      {*b};
            const auto varVal {varVals[v->level - 1]};
            
            ++b;
            
            v->forwardStar[varVal].target  = *b;
            v->forwardStar[!varVal].target = xLeaf;
        }

        const auto v      {*b};
        const auto varVal {varVals[v->level - 1]};

        v->forwardStar[varVal].target  = valLeaf;
        v->forwardStar[!varVal].target = xLeaf;
        
        std::map<const vertex*, log_val_t> leafToVal 
        { 
            {valLeaf, fVal}
          , {xLeaf, X} 
        };

        return bdd_t 
        {
            relevantVariables.front()
          , leafLevel - 1
          , std::move(leafToVal)
        };
    }

    template<class VertexData, class ArcData>
    auto bdds_from_pla<VertexData, ArcData>::merge_diagrams
        (std::vector<bdd_t> diagrams) -> bdd_t
    {
        const auto numOfSteps 
        {
            static_cast<size_t>(std::ceil(std::log2(diagrams.size())))
        };

        size_t diagramCount {diagrams.size()};

        std::vector<bdd_t> auxDiagrams2((diagrams.size() >> 1) + 1);

        std::vector<bdd_t>* src {&diagrams};
        std::vector<bdd_t>* dst {&auxDiagrams2};

        for (size_t step {0}; step < numOfSteps; ++step)
        {
            diagramCount = (diagramCount >> 1) + (diagramCount & 1);

            // parallelizable for
            for (int32_t i {0}; i < diagramCount; ++i)
            {
                if (i < diagramCount - 1)
                {
                    (*dst)[i] = (*src)[i << 1] || (*src)[(i << 1) + 1];
                }
                else // TODO toto by možno išlo aj bez toho ifu vnútri
                {
                    (*dst)[i] = (*src)[i];
                }
            }

            std::swap(src, dst);
        }

        return dst->front();
    }
}

#endif