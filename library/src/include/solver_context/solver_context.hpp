/*
   DDS, a bridge double dummy solver.

   Public SPM-safe interface for SolverContext.
   No private implementation headers are included here.
   Internal solver code uses SolverContextImpl (solver_context_impl.hpp).
*/

#pragma once

#include <memory>

enum class TTKind { Small, Large };

/**
 * @brief Configuration options for SolverContext instances.
 */
struct SolverConfig
{
  TTKind tt_kind_ = TTKind::Large;
  int tt_mem_default_mb_ = 0;
  int tt_mem_maximum_mb_ = 0;
};

class SolverContextImpl;

/**
 * @brief Instance-scoped solver context for DDS.
 *
 * Create one SolverContext per solver invocation (or reuse across multiple
 * calls for TT persistence). Not thread-safe — use one instance per thread.
 *
 * @note Prefer the default constructor. Supply SolverConfig only to override
 *       transposition table kind or memory limits.
 */
class SolverContext
{
public:
  explicit SolverContext(SolverConfig cfg = {});

  SolverContext(SolverContext&&) noexcept;
  SolverContext& operator=(SolverContext&&) noexcept;
  SolverContext(const SolverContext&) = delete;
  SolverContext& operator=(const SolverContext&) = delete;

  ~SolverContext();

  auto config() const -> const SolverConfig&;

  auto configure_tt(TTKind kind, int defMB, int maxMB) -> void;
  auto reset_for_solve() const -> void;
  auto reset_best_moves_lite() const -> void;
  auto clear_tt() const -> void;
  auto resize_tt(int defMB, int maxMB) const -> void;
  auto dispose_trans_table() const -> void;

  // Internal escape hatch — not part of the consumer API.
  auto impl() -> SolverContextImpl&;
  auto impl() const -> const SolverContextImpl&;

private:
  std::unique_ptr<SolverContextImpl> pimpl_;
};
