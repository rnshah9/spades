//***************************************************************************
//* Copyright (c) 2021 Saint Petersburg State University
//* All Rights Reserved
//* See file LICENSE for details.
//***************************************************************************

#pragma once

#include "binning.hpp"
#include "id_map.hpp"

#include "assembly_graph/core/graph.hpp"

namespace bin_stats {

using AlphaAssignment = adt::id_map<double, debruijn_graph::EdgeId>;

class AlphaAssigner {
  public:
    explicit AlphaAssigner(const debruijn_graph::Graph &g)
        : g_(g) {}
    virtual ~AlphaAssigner() = default;

    virtual AlphaAssignment GetAlphaAssignment(const SoftBinsAssignment &state) const = 0;

  protected:
    const debruijn_graph::Graph &g_;
};

class PropagationAssigner : public AlphaAssigner {
  public:
    explicit PropagationAssigner(const debruijn_graph::Graph &g)
        : AlphaAssigner(g) {}

    AlphaAssignment GetAlphaAssignment(const SoftBinsAssignment &state) const override;

  private:
    using AlphaAssigner::g_;
};

class AlphaCorrector: public AlphaAssigner {
  public:
    using EdgeId = debruijn_graph::EdgeId;
    using Graph = debruijn_graph::Graph;

    AlphaCorrector(const Graph &g, double labeled_alpha);
    AlphaCorrector(const debruijn_graph::Graph &g, const AlphaAssignment &distance_coeffs, double labeled_alpha);

    AlphaAssignment GetAlphaAssignment(const SoftBinsAssignment &state) const override;
  private:
    AlphaAssignment distance_coeffs_;
    double labeled_alpha_;
};

}