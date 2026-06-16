/*
   DDS, a bridge double dummy solver.

   Copyright (C) 2006-2014 by Bo Haglund /
   2014-2018 by Bo Haglund & Soren Hein.

   See LICENSE and README.
*/

#pragma once

#include <api/dds.h>
#include <system/memory.hpp>
#include <solver_context/solver_context_impl.hpp>
#include <memory>


void SetDeal(const std::shared_ptr<ThreadData>& thrp);

void SetDealTables(SolverContextImpl& ctx);

void InitWinners(
  const Deal& dl,
  Pos& posPoint,
  const std::shared_ptr<ThreadData>& thrp);

void CloseDebugFiles();
