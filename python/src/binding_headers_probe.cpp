// Compile-only proof that belief_evaluation's public headers, now namespaced
// under dds::belief_evaluation, coexist with bindings.cpp's own include
// block (mirrored verbatim below) in one translation unit. No pybind11
// wrappers, no module definition, no binding code -- just the includes,
// plus one reference to a type from each side so neither include is dead
// code a compiler could discard unnoticed.
//
// bindings.cpp itself has no ordering trick available to it -- it is one
// file that already binds the whole dds API, with nothing to put "first" --
// so this is the check that mattered: if this translation unit did not
// compile, real bindings could not be written against this module at all
// until this gap was fixed first.
#include <algorithm>
#include <array>
#include <cstddef>
#include <memory>
#include <stdexcept>
#include <string>

#include <pybind11/pybind11.h>

#include <api/calc_par.hpp>
#include <dds/dds.hpp>
#include <pbn.hpp>
#include <solver_context/solver_context.hpp>

#include "converters.hpp"

#include <belief_evaluation/evaluate.hpp>

namespace
{
    // References one type each side declares, so neither include is dead:
    // ::Deal is dds's own, unqualified and unaffected by this module's
    // namespace; dds::belief_evaluation::EvaluationResult is this module's.
    [[maybe_unused]] auto probe(Deal const& deal) -> dds::belief_evaluation::EvaluationResult
    {
        (void)deal;
        return dds::belief_evaluation::EvaluationResult{};
    }
}
