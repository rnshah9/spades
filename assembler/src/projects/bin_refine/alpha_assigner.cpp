//***************************************************************************
//* Copyright (c) 2021 Saint Petersburg State University
//* All Rights Reserved
//* See file LICENSE for details.
//***************************************************************************

#include "alpha_assigner.hpp"
#include "binning.hpp"
#include "binning_assignment_strategy.hpp"

namespace bin_stats {

AlphaAssignment PropagationAssigner::GetAlphaAssignment(const SoftBinsAssignment &origin_state) const {
    AlphaAssignment ealpha(g_.max_eid());
    // Calculate the regularization coefficients
    for (debruijn_graph::EdgeId e : g_.canonical_edges()) {
        double alpha = 0;
        const EdgeLabels &edge_labels = origin_state.at(e);
        alpha = 1.0;
        if (edge_labels.is_binned && !edge_labels.is_repetitive) {
            alpha = 0;
        }
        ealpha.emplace(e, alpha);
        ealpha.emplace(g_.conjugate(e), alpha);
    }
    return ealpha;
}
AlphaAssignment AlphaCorrector::GetAlphaAssignment(const SoftBinsAssignment &origin_state) const {
    bool has_mask = distance_coeffs_.begin() == distance_coeffs_.end();
    AlphaAssignment ealpha(g_.max_eid());
    //Calculate the regularization coefficients
    for (EdgeId e : g_.canonical_edges()) {
        double alpha = 0;
        const EdgeLabels &edge_labels = origin_state.at(e);

        // Formula for correction: next_probs[i] = alpha[e] * rw[e] * \sum{neighbour} (rd[neighbour] * cur_probs[neighbour]) + (1 - alpha[e]) * origin_probs[e]
        // Therefore:

        // If alpha is zero, then original binning will be used
        // If alpha is one, then original binning will be ignored
        // Otherwise, alpha is used as a regularization coefficient for binning propagation
        // If the edge is binned and is not repetitive, then we use labelled alpha
        // (zero in case of simple propagation and some pre-defined value
        // otherwise), otherwise we do not trust input binning and set alpha to one.
        // Also, make alpha dependent on the edge length and distance from the binned edges.
        alpha = 1.0;
        if (edge_labels.is_binned && !edge_labels.is_repetitive) {
            size_t l = g_.length(e), thr = 1000;
            double coef = (l > thr ? 1.0 : ::log(l) / ::log(thr));
            double mask_coef = (has_mask ? distance_coeffs_.at(e) : 1.0);
            alpha = labeled_alpha_ * coef * mask_coef;
        }
        ealpha.emplace(e, alpha);
        ealpha.emplace(g_.conjugate(e), alpha);
    }
    return ealpha;
}
AlphaCorrector::AlphaCorrector(const debruijn_graph::Graph &g,
                               const AlphaAssignment &distance_coeffs,
                               double labeled_alpha) :
    AlphaAssigner(g), distance_coeffs_(distance_coeffs), labeled_alpha_(labeled_alpha) {}

AlphaCorrector::AlphaCorrector(const debruijn_graph::Graph &g, double labeled_alpha) : AlphaAssigner(g),
                                                                                       distance_coeffs_(0),
                                                                                       labeled_alpha_(labeled_alpha) {}
}
