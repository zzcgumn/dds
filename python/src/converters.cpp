#include "converters.hpp"

#include <algorithm>
#include <cstring>

#include <pybind11/pybind11.h>
#include <string>
#include <vector>

namespace py = pybind11;
namespace be = dds::belief_evaluation;

namespace dds3_python
{

constexpr int MaxSuitBitmask = 0x7FFC;

auto sequence_to_int_vector(
    const py::sequence& values,
    const std::size_t expected_size,
    const std::string& field_name) -> std::vector<int>
{
    if (values.size() != expected_size) {
        throw py::value_error(field_name + " must have size " + std::to_string(expected_size));
    }

    std::vector<int> result;
    result.reserve(expected_size);
    for (const py::handle value : values) {
        result.push_back(py::cast<int>(value));
    }

    return result;
}

auto sequence_to_bounded_int_vector(
    const py::sequence& values,
    const std::size_t expected_size,
    const int minimum_value,
    const int maximum_value,
    const std::string& field_name) -> std::vector<int>
{
    const auto result = sequence_to_int_vector(values, expected_size, field_name);
    for (const int value : result) {
        if (value < minimum_value || value > maximum_value) {
            throw py::value_error(
                field_name + " has invalid value " + std::to_string(value) +
                " (expected range " + std::to_string(minimum_value) + ".." +
                std::to_string(maximum_value) + ")");
        }
    }

    return result;
}

auto dict_to_deal(const py::dict& deal_input) -> Deal
{
    Deal deal{};

    const int trump = py::cast<int>(deal_input["trump"]);
    if (trump < 0 || trump > DDS_STRAINS - 1) {
        throw py::value_error(
            "trump has invalid value " + std::to_string(trump) +
            " (expected range 0.." + std::to_string(DDS_STRAINS - 1) + ")");
    }

    const int first = py::cast<int>(deal_input["first"]);
    if (first < 0 || first > DDS_HANDS - 1) {
        throw py::value_error(
            "first has invalid value " + std::to_string(first) +
            " (expected range 0.." + std::to_string(DDS_HANDS - 1) + ")");
    }

    deal.trump = trump;
    deal.first = first;
    const auto trick_suit = sequence_to_bounded_int_vector(
        py::cast<py::sequence>(deal_input["current_trick_suit"]),
        3,
        0,
        DDS_SUITS - 1,
        "current_trick_suit");
    const auto trick_rank = sequence_to_bounded_int_vector(
        py::cast<py::sequence>(deal_input["current_trick_rank"]),
        3,
        0,
        14,
        "current_trick_rank");
    for (const int value : trick_rank) {
        if (value != 0 && (value < 2 || value > 14)) {
            throw py::value_error(
                "current_trick_rank has invalid value " + std::to_string(value) +
                " (expected 0 or 2..14)");
        }
    }

    for (int i = 0; i < 3; ++i) {
        deal.currentTrickSuit[i] = trick_suit[static_cast<std::size_t>(i)];
        deal.currentTrickRank[i] = trick_rank[static_cast<std::size_t>(i)];
    }

    const auto remain_cards_rows = py::cast<py::sequence>(deal_input["remain_cards"]);
    if (remain_cards_rows.size() != DDS_HANDS) {
        throw py::value_error(
            "remain_cards must have " + std::to_string(DDS_HANDS) + " rows");
    }

    for (int hand = 0; hand < DDS_HANDS; ++hand) {
        const auto row = py::cast<py::sequence>(remain_cards_rows[hand]);
        if (row.size() != DDS_SUITS) {
            throw py::value_error(
                "each remain_cards row must have " + std::to_string(DDS_SUITS) + " values");
        }
        for (int suit = 0; suit < DDS_SUITS; ++suit) {
            const int value = py::cast<int>(row[suit]);
            if (value < 0 || value > MaxSuitBitmask) {
                throw py::value_error(
                    "remain_cards has invalid value " + std::to_string(value) +
                    " (expected range 0..0x7FFC)");
            }
            deal.remainCards[hand][suit] = static_cast<unsigned int>(value);
        }
    }

    return deal;
}

// The reverse of dict_to_deal, to the same dict shape docs/python_interface.md
// documents -- so dict_to_deal(deal_to_dict(d)) round-trips for any Deal a
// caller can legitimately hold, and deal_to_dict(dict_to_deal(dict)) does
// too for any dict dict_to_deal accepts.
auto deal_to_dict(const Deal& deal) -> py::dict
{
    py::dict result;
    result["trump"] = deal.trump;
    result["first"] = deal.first;

    py::tuple trick_suit(3);
    py::tuple trick_rank(3);
    for (int i = 0; i < 3; ++i) {
        trick_suit[i] = deal.currentTrickSuit[i];
        trick_rank[i] = deal.currentTrickRank[i];
    }
    result["current_trick_suit"] = trick_suit;
    result["current_trick_rank"] = trick_rank;

    py::list remain_cards;
    for (int hand = 0; hand < DDS_HANDS; ++hand) {
        py::list row;
        for (int suit = 0; suit < DDS_SUITS; ++suit) {
            row.append(static_cast<int>(deal.remainCards[hand][suit]));
        }
        remain_cards.append(row);
    }
    result["remain_cards"] = remain_cards;

    return result;
}

auto pbn_to_deal(
    const std::string& remain_cards,
    const int trump,
    const int first,
    const py::sequence& current_trick_suit,
    const py::sequence& current_trick_rank) -> DealPBN
{
    // Validate trump and first (same validation as dict_to_deal)
    if (trump < 0 || trump > DDS_STRAINS - 1) {
        throw py::value_error(
            "trump has invalid value " + std::to_string(trump) +
            " (expected range 0.." + std::to_string(DDS_STRAINS - 1) + ")");
    }
    if (first < 0 || first > DDS_HANDS - 1) {
        throw py::value_error(
            "first has invalid value " + std::to_string(first) +
            " (expected range 0.." + std::to_string(DDS_HANDS - 1) + ")");
    }

    // Validate remain_cards length (PBN format expects specific size)
    constexpr std::size_t expected_size = sizeof(DealPBN::remainCards) - 1U;
    if (remain_cards.size() > expected_size) {
        throw py::value_error(
            "remain_cards PBN string is too long (" + std::to_string(remain_cards.size()) +
            " bytes, maximum " + std::to_string(expected_size) + ")");
    }

    DealPBN deal{};
    deal.trump = trump;
    deal.first = first;

    const auto trick_suit = sequence_to_bounded_int_vector(
        current_trick_suit,
        3,
        0,
        DDS_SUITS - 1,
        "current_trick_suit");
    const auto trick_rank = sequence_to_bounded_int_vector(
        current_trick_rank,
        3,
        0,
        14,
        "current_trick_rank");
    for (const int value : trick_rank) {
        if (value != 0 && (value < 2 || value > 14)) {
            throw py::value_error(
                "current_trick_rank has invalid value " + std::to_string(value) +
                " (expected 0 or 2..14)");
        }
    }
    for (int i = 0; i < 3; ++i) {
        deal.currentTrickSuit[i] = trick_suit[static_cast<std::size_t>(i)];
        deal.currentTrickRank[i] = trick_rank[static_cast<std::size_t>(i)];
    }

    std::memset(deal.remainCards, 0, sizeof(deal.remainCards));
    const std::size_t copy_size = std::min(remain_cards.size(), sizeof(deal.remainCards) - 1U);
    std::memcpy(deal.remainCards, remain_cards.c_str(), copy_size);
    deal.remainCards[copy_size] = '\0';

    return deal;
}

auto dict_to_dd_table_deal(const py::dict& table_input) -> DdTableDeal
{
    DdTableDeal table_deal{};
    const auto cards_rows = py::cast<py::sequence>(table_input["cards"]);
    if (cards_rows.size() != DDS_HANDS) {
        throw py::value_error(
            "cards must have " + std::to_string(DDS_HANDS) + " rows");
    }

    for (int hand = 0; hand < DDS_HANDS; ++hand) {
        const auto row = py::cast<py::sequence>(cards_rows[hand]);
        if (row.size() != DDS_SUITS) {
            throw py::value_error(
                "each cards row must have " + std::to_string(DDS_SUITS) + " values");
        }
        for (int suit = 0; suit < DDS_SUITS; ++suit) {
            const int value = py::cast<int>(row[suit]);
            if (value < 0 || value > MaxSuitBitmask) {
                throw py::value_error(
                    "cards has invalid value " + std::to_string(value) +
                    " (expected range 0..0x7FFC)");
            }
            table_deal.cards[hand][suit] = static_cast<unsigned int>(value);
        }
    }

    return table_deal;
}

auto dict_to_dd_table_results(const py::dict& table_input) -> DdTableResults
{
    DdTableResults table_results{};
    const auto table_rows = py::cast<py::sequence>(table_input["res_table"]);
    if (table_rows.size() != DDS_STRAINS) {
        throw py::value_error(
            "res_table must have " + std::to_string(DDS_STRAINS) + " rows");
    }

    for (int strain = 0; strain < DDS_STRAINS; ++strain) {
        const auto row = py::cast<py::sequence>(table_rows[strain]);
        if (row.size() != DDS_HANDS) {
            throw py::value_error(
                "each res_table row must have " + std::to_string(DDS_HANDS) + " values");
        }
        for (int hand = 0; hand < DDS_HANDS; ++hand) {
            table_results.res_table[strain][hand] = py::cast<int>(row[hand]);
        }
    }

    return table_results;
}

auto future_tricks_to_dict(const FutureTricks& future_tricks) -> py::dict
{
    py::dict result;
    result["nodes"] = future_tricks.nodes;
    result["cards"] = future_tricks.cards;
    
    // Convert arrays to tuples using loops for maintainability
    py::tuple suit(13);
    py::tuple rank(13);
    py::tuple equals(13);
    py::tuple score(13);
    for (int i = 0; i < 13; ++i) {
        suit[i] = future_tricks.suit[i];
        rank[i] = future_tricks.rank[i];
        equals[i] = future_tricks.equals[i];
        score[i] = future_tricks.score[i];
    }
    result["suit"] = suit;
    result["rank"] = rank;
    result["equals"] = equals;
    result["score"] = score;

    return result;
}

auto dd_table_results_to_dict(const DdTableResults& table_results) -> py::dict
{
    py::list rows;
    for (int strain = 0; strain < DDS_STRAINS; ++strain) {
        py::list row;
        for (int hand = 0; hand < DDS_HANDS; ++hand) {
            row.append(table_results.res_table[strain][hand]);
        }
        rows.append(row);
    }

    py::dict result;
    result["res_table"] = rows;
    return result;
}

auto par_results_to_dict(const ParResults& par_results) -> py::dict
{
    py::list par_score;
    py::list par_contracts;

    par_score.append(std::string(par_results.par_score[0]));
    par_score.append(std::string(par_results.par_score[1]));

    par_contracts.append(std::string(par_results.par_contracts_string[0]));
    par_contracts.append(std::string(par_results.par_contracts_string[1]));

    py::dict result;
    result["par_score"] = par_score;
    result["par_contracts_string"] = par_contracts;
    return result;
}

auto solved_play_to_dict(const SolvedPlay& solved_play) -> py::dict
{
    py::list tricks;
    const int count = std::max(0, std::min(solved_play.number, 53));
    for (int i = 0; i < count; ++i) {
        tricks.append(solved_play.tricks[i]);
    }

    py::dict result;
    // Report the count actually returned so len(tricks) == number always holds,
    // even if DDS ever yields an out-of-range solved_play.number.
    result["number"] = count;
    result["tricks"] = tricks;
    return result;
}

auto list_to_dd_table_deals_pbn(
    const py::list& deals_pbn,
    const std::size_t max_tables) -> DdTableDealsPBN
{
    const auto table_count = static_cast<std::size_t>(deals_pbn.size());

    if (table_count > max_tables) {
        throw py::value_error(
            "Number of tables (" + std::to_string(table_count) +
            ") exceeds maximum (" + std::to_string(max_tables) + ")");
    }

    DdTableDealsPBN result{};
    result.no_of_tables = static_cast<int>(table_count);

    for (std::size_t i = 0; i < table_count; ++i) {
        const auto pbn_str = py::cast<std::string>(deals_pbn[i]);
        if (pbn_str.length() >= 80) {
            throw py::value_error(
                "PBN string at index " + std::to_string(i) +
                " is too long (max 79 characters)");
        }
        std::memset(result.deals[i].cards, 0, sizeof(result.deals[i].cards));
        std::memcpy(result.deals[i].cards, pbn_str.data(), pbn_str.size());
        result.deals[i].cards[pbn_str.size()] = '\0';
    }

    return result;
}

auto dd_tables_res_to_list(const DdTablesRes& tables_res, const int num_tables) -> py::list
{
    const int max_tables = MAXNOOFTABLES * DDS_STRAINS;
    const int count = std::max(0, std::min(num_tables, max_tables));

    py::list result;
    for (int i = 0; i < count; ++i) {
        result.append(dd_table_results_to_dict(tables_res.results[i]));
    }
    return result;
}

auto all_par_results_to_list(const AllParResults& all_par_results, const int num_tables) -> py::list
{
    // AllParResults::par_results is sized MAXNOOFTABLES, so clamp num_tables
    // to avoid out-of-bounds access
    const int max_tables = MAXNOOFTABLES;
    const int count = std::max(0, std::min(num_tables, max_tables));

    py::list result;
    for (int i = 0; i < count; ++i) {
        result.append(par_results_to_dict(all_par_results.par_results[i]));
    }
    return result;
}

namespace
{
    auto depth_sample_stats_to_dict(const be::DepthSampleStats& stats) -> py::dict
    {
        py::dict result;
        result["nodes"] = stats.nodes;
        result["layout_sum"] = stats.layout_sum;
        result["layout_min"] = stats.layout_min;
        return result;
    }

    auto depth_replenishment_stats_to_dict(const be::DepthReplenishmentStats& stats) -> py::dict
    {
        py::dict result;
        result["attempted"] = stats.attempted;
        result["succeeded"] = stats.succeeded;
        result["layouts_added"] = stats.layouts_added;
        result["at_calls"] = stats.at_calls;
        return result;
    }

    auto evaluation_counters_to_dict(const be::EvaluationCounters& counters) -> py::dict
    {
        py::dict result;
        result["nodes_visited"] = counters.nodes_visited;
        result["tier1_made_cuts"] = counters.tier1_made_cuts;
        result["tier1_dead_cuts"] = counters.tier1_dead_cuts;
        result["tier2_cuts"] = counters.tier2_cuts;

        // Not summarised, not turned into a ratio on the way out -- a
        // ratio cannot be summed across depths or runs, which is exactly
        // why the C++ type stores counts rather than a scan-to-hit figure
        // itself. An index beyond either list's own length means "no node
        // was ever visited at this depth", distinct from an entry present
        // with nodes == 0 -- the vectors are grown on demand for exactly
        // this reason, and that distinction survives here unchanged: this
        // list is exactly as long as the C++ vector, no padding either way.
        py::list sample_size_by_depth;
        for (const be::DepthSampleStats& depth : counters.sample_size_by_depth) {
            sample_size_by_depth.append(depth_sample_stats_to_dict(depth));
        }
        result["sample_size_by_depth"] = sample_size_by_depth;

        py::list replenishment_by_depth;
        for (const be::DepthReplenishmentStats& depth : counters.replenishment_by_depth) {
            replenishment_by_depth.append(depth_replenishment_stats_to_dict(depth));
        }
        result["replenishment_by_depth"] = replenishment_by_depth;

        return result;
    }

    // The root node only -- EvaluateOptions::retain_root never holds a
    // retained tree (a deliberate choice, not an oversight: see that
    // field's own doxygen), and a caller seeing "retained root" in a
    // result would otherwise reasonably assume a tree is reachable from
    // it. kappa never crosses, for the same reason a BeliefView's own
    // posterior is the normalised form and not kappa (see belief_view.hpp)
    // -- it is the evaluator's own sample-weight bookkeeping, not
    // something declarer (or a caller reading the result) ever needs.
    auto belief_node_to_dict(const be::BeliefNode& node) -> py::dict
    {
        py::dict result;

        py::list layouts;
        for (const Deal& layout : node.layouts) {
            layouts.append(deal_to_dict(layout));
        }
        result["layouts"] = layouts;

        py::list p;
        for (const be::Probability& weight : node.p) {
            p.append(weight);
        }
        result["p"] = p;

        py::list root_keys;
        for (const std::uint64_t& key : node.root_keys) {
            root_keys.append(key);
        }
        result["root_keys"] = root_keys;

        result["is_sample"] = node.is_sample;
        result["no_more_available"] = node.no_more_available;
        return result;
    }
}  // namespace

// Dict out for the whole result, matching this file's existing idiom (see
// this function's own doxygen in converters.hpp for why the direction
// matters here and not for a Deal or an ObservationState). Value and
// error are mutually exclusive on the C++ type (EvaluationResult::error's
// own doxygen: "meaningful only when by_strategy is empty") and stay that
// way here: exactly one of "by_strategy" or "error" is ever present on
// the returned dict, never both and never neither.
auto evaluation_result_to_dict(const be::EvaluationResult& result) -> py::dict
{
    py::dict out;

    if (result.error.has_value()) {
        const be::EvaluationError& error = *result.error;
        py::dict error_dict;
        error_dict["validation"] = error.validation;
        error_dict["callback"] = error.callback;
        error_dict["seat"] = error.seat;
        error_dict["layout"] = deal_to_dict(error.layout);
        error_dict["root_failure"] = error.root_failure;
        out["error"] = error_dict;
        return out;
    }

    py::dict by_strategy;
    for (const auto& [id, value] : result.by_strategy) {
        py::dict entry;
        entry["p_make"] = value.p_make;

        // root_children: alternatives, not a partition, at a declarer
        // root (p_make equals whichever entry pi actually chose, not
        // their sum); a genuine partition summing to p_make at a
        // defender root; empty at a terminal root, or one where declarer
        // has already banked every trick the contract needs. Summing
        // these and comparing to p_make agrees sometimes and not others
        // -- read EvaluationValue::root_children's own doxygen before
        // drawing a conclusion from a mismatch.
        py::list root_children;
        for (const be::RootChildValue& child : value.root_children) {
            root_children.append(py::make_tuple(child.card, child.value));
        }
        entry["root_children"] = root_children;

        if (value.retained_root.has_value()) {
            entry["retained_root"] = belief_node_to_dict(*value.retained_root);
        }
        if (value.counters.has_value()) {
            entry["counters"] = evaluation_counters_to_dict(*value.counters);
        }

        by_strategy[py::cast(id)] = entry;
    }
    out["by_strategy"] = by_strategy;
    return out;
}

}  // namespace dds3_python
