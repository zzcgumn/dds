#pragma once

#include <string>
#include <vector>

#include <pybind11/pytypes.h>

#include <dds/dds.hpp>

#include <belief_evaluation/evaluate.hpp>

namespace dds3_python
{

auto sequence_to_int_vector(
    const pybind11::sequence& values,
    std::size_t expected_size,
    const std::string& field_name) -> std::vector<int>;

auto sequence_to_bounded_int_vector(
    const pybind11::sequence& values,
    std::size_t expected_size,
    int minimum_value,
    int maximum_value,
    const std::string& field_name) -> std::vector<int>;

auto dict_to_deal(const pybind11::dict& deal_input) -> Deal;
auto deal_to_dict(const Deal& deal) -> pybind11::dict;
auto pbn_to_deal(
    const std::string& remain_cards,
    int trump,
    int first,
    const pybind11::sequence& current_trick_suit,
    const pybind11::sequence& current_trick_rank) -> DealPBN;
auto dict_to_dd_table_deal(const pybind11::dict& table_input) -> DdTableDeal;
auto dict_to_dd_table_results(const pybind11::dict& table_input) -> DdTableResults;

auto future_tricks_to_dict(const FutureTricks& future_tricks) -> pybind11::dict;
auto dd_table_results_to_dict(const DdTableResults& table_results) -> pybind11::dict;
auto par_results_to_dict(const ParResults& par_results) -> pybind11::dict;
auto solved_play_to_dict(const SolvedPlay& solved_play) -> pybind11::dict;

auto list_to_dd_table_deals_pbn(
    const pybind11::list& deals_pbn,
    std::size_t max_tables) -> DdTableDealsPBN;

auto dd_tables_res_to_list(const DdTablesRes& tables_res, int num_tables) -> pybind11::list;

auto all_par_results_to_list(const AllParResults& all_par_results, int num_tables) -> pybind11::list;

// dict out, matching this file's own existing idiom -- unlike
// ObservationState or BeliefView (bound as read-only classes, since they
// are handed *to* a callback and materialising a dict per call would be
// the belief-set-copy cost those bindings exist to avoid), an
// EvaluationResult is handed *out*, once, at the end of one evaluate()
// call: no per-call cost to avoid, and a caller of the existing bindings
// already expects a dict back. Never constructed by a caller, so no
// matching dict_to_* exists or is needed.
auto evaluation_result_to_dict(const dds::belief_evaluation::EvaluationResult& result) -> pybind11::dict;

}  // namespace dds3_python
