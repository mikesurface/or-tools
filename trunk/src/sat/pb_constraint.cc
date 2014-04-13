// Copyright 2010-2013 Google
// Licensed under the Apache License, Version 2.0 (the "License");
// you may not use this file except in compliance with the License.
// You may obtain a copy of the License at
//
//     http://www.apache.org/licenses/LICENSE-2.0
//
// Unless required by applicable law or agreed to in writing, software
// distributed under the License is distributed on an "AS IS" BASIS,
// WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
// See the License for the specific language governing permissions and
// limitations under the License.
#include "sat/pb_constraint.h"

#include "util/saturated_arithmetic.h"

namespace operations_research {
namespace sat {

namespace {

bool LiteralComparator(const LiteralWithCoeff& a, const LiteralWithCoeff& b) {
  return a.literal.Index() < b.literal.Index();
}

bool CoeffComparator(const LiteralWithCoeff& a, const LiteralWithCoeff& b) {
  if (a.coefficient == b.coefficient) {
    return a.literal.Index() < b.literal.Index();
  }
  return a.coefficient < b.coefficient;
}

}  // namespace

bool ComputeBooleanLinearExpressionCanonicalForm(std::vector<LiteralWithCoeff>* cst,
                                                 Coefficient* bound_shift,
                                                 Coefficient* max_value) {
  // Note(user): For some reason, the IntType checking doesn't work here ?! that
  // is a bit worrying, but the code seems to behave correctly.
  *bound_shift = 0;
  *max_value = 0;

  // First, sort by literal to remove duplicate literals.
  // This also remove term with a zero coefficient.
  std::sort(cst->begin(), cst->end(), LiteralComparator);
  int index = 0;
  LiteralWithCoeff* representative = nullptr;
  for (int i = 0; i < cst->size(); ++i) {
    const LiteralWithCoeff current = (*cst)[i];
    if (current.coefficient == 0) continue;
    if (representative != nullptr &&
        current.literal.Variable() == representative->literal.Variable()) {
      if (current.literal == representative->literal) {
        if (!SafeAddInto(current.coefficient, &(representative->coefficient)))
          return false;
      } else {
        // Here current_literal is equal to (1 - representative).
        if (!SafeAddInto(-current.coefficient, &(representative->coefficient)))
          return false;
        if (!SafeAddInto(-current.coefficient, bound_shift)) return false;
      }
    } else {
      if (representative != nullptr && representative->coefficient == 0) {
        --index;
      }
      (*cst)[index] = current;
      representative = &((*cst)[index]);
      ++index;
    }
  }
  if (representative != nullptr && representative->coefficient == 0) {
    --index;
  }
  cst->resize(index);

  // Then, make all coefficients positive by replacing a term "-c x" into
  // "c(1-x) - c" which is the same as "c(not x) - c".
  for (int i = 0; i < cst->size(); ++i) {
    const LiteralWithCoeff current = (*cst)[i];
    if (current.coefficient < 0) {
      if (!SafeAddInto(-current.coefficient, bound_shift)) return false;
      (*cst)[i].coefficient = -current.coefficient;
      (*cst)[i].literal = current.literal.Negated();
    }
    if (!SafeAddInto((*cst)[i].coefficient, max_value)) return false;
  }

  // Finally sort by increasing coefficients.
  std::sort(cst->begin(), cst->end(), CoeffComparator);
  DCHECK_GE(*max_value, 0);
  return true;
}

// TODO(user): Also check for no duplicates literals + unit tests.
bool BooleanLinearExpressionIsCanonical(const std::vector<LiteralWithCoeff>& cst) {
  Coefficient previous(1);
  for (LiteralWithCoeff term : cst) {
    if (term.coefficient < previous) return false;
    previous = term.coefficient;
  }
  return true;
}

// TODO(user): Use more complex simplification like dividing by the gcd of
// everyone and using less different coefficients if possible.
void SimplifyCanonicalBooleanLinearConstraint(std::vector<LiteralWithCoeff>* cst,
                                              Coefficient* rhs) {
  // Replace all coefficient >= rhs by rhs + 1 (these literal must actually be
  // false). Note that the linear sum of literals remains canonical.
  //
  // TODO(user): It is probably better to remove these literals and have other
  // constraint setting them to false from the symmetry finder perspective.
  for (LiteralWithCoeff& x : *cst) {
    if (x.coefficient > *rhs) x.coefficient = *rhs + 1;
  }
}

Coefficient ComputeCanonicalRhs(Coefficient upper_bound,
                                Coefficient bound_shift,
                                Coefficient max_value) {
  Coefficient rhs = upper_bound;
  if (!SafeAddInto(bound_shift, &rhs)) {
    if (bound_shift > 0) {
      // Positive overflow. The constraint is trivially true.
      // This is because the canonical linear expression is in [0, max_value].
      return max_value;
    } else {
      // Negative overflow. The constraint is infeasible.
      return Coefficient(-1);
    }
  }
  if (rhs < 0) return Coefficient(-1);
  return std::min(max_value, rhs);
}

Coefficient ComputeNegatedCanonicalRhs(Coefficient lower_bound,
                                       Coefficient bound_shift,
                                       Coefficient max_value) {
  // The new bound is "max_value - (lower_bound + bound_shift)", but we must
  // pay attention to possible overflows.
  Coefficient shifted_lb = lower_bound;
  if (!SafeAddInto(bound_shift, &shifted_lb)) {
    if (bound_shift > 0) {
      // Positive overflow. The constraint is infeasible.
      return Coefficient(-1);
    } else {
      // Negative overflow. The constraint is trivialy satisfiable.
      return max_value;
    }
  }
  if (shifted_lb <= 0) {
    // If shifted_lb <= 0 then the constraint is trivialy satisfiable. We test
    // this so we are sure that max_value - shifted_lb doesn't overflow below.
    return max_value;
  }
  return max_value - shifted_lb;
}

bool CanonicalBooleanLinearProblem::AddLinearConstraint(
    bool use_lower_bound, Coefficient lower_bound, bool use_upper_bound,
    Coefficient upper_bound, std::vector<LiteralWithCoeff>* cst) {
  // Canonicalize the linear expression of the constraint.
  Coefficient bound_shift;
  Coefficient max_value;
  if (!ComputeBooleanLinearExpressionCanonicalForm(cst, &bound_shift,
                                                   &max_value)) {
    return false;
  }
  if (use_upper_bound) {
    const Coefficient rhs =
        ComputeCanonicalRhs(upper_bound, bound_shift, max_value);
    if (!AddConstraint(*cst, max_value, rhs)) return false;
  }
  if (use_lower_bound) {
    // We transform the constraint into an upper-bounded one.
    for (int i = 0; i < cst->size(); ++i) {
      (*cst)[i].literal = (*cst)[i].literal.Negated();
    }
    const Coefficient rhs =
        ComputeNegatedCanonicalRhs(lower_bound, bound_shift, max_value);
    if (!AddConstraint(*cst, max_value, rhs)) return false;
  }
  return true;
}

bool CanonicalBooleanLinearProblem::AddConstraint(
    const std::vector<LiteralWithCoeff>& cst, Coefficient max_value,
    Coefficient rhs) {
  if (rhs < 0) return false;          // Trivially unsatisfiable.
  if (rhs >= max_value) return true;  // Trivially satisfiable.
  constraints_.emplace_back(cst.begin(), cst.end());
  rhs_.push_back(rhs);
  SimplifyCanonicalBooleanLinearConstraint(&constraints_.back(), &rhs_.back());
  return true;
}

UpperBoundedLinearConstraint::UpperBoundedLinearConstraint(
    const std::vector<LiteralWithCoeff>& cst, ResolutionNode* node)
    : node_(node) {
  DCHECK(!cst.empty());
  DCHECK(std::is_sorted(cst.begin(), cst.end(), CoeffComparator));
  literals_.reserve(cst.size());
  for (LiteralWithCoeff term : cst) {
    if (term.coefficient == 0) continue;
    if (coeffs_.empty() || term.coefficient != coeffs_.back()) {
      coeffs_.push_back(term.coefficient);
      starts_.push_back(literals_.size());
    }
    literals_.push_back(term.literal);
  }

  // Sentinel.
  starts_.push_back(literals_.size());
}

bool UpperBoundedLinearConstraint::HasIdenticalTerms(
    const std::vector<LiteralWithCoeff>& cst) {
  if (cst.size() != literals_.size()) return false;
  int literal_index = 0;
  int coeff_index = 0;
  for (LiteralWithCoeff term : cst) {
    if (literals_[literal_index] != term.literal) return false;
    if (coeffs_[coeff_index] != term.coefficient) return false;
    ++literal_index;
    if (literal_index == starts_[coeff_index + 1]) {
      ++coeff_index;
    }
  }
  return true;
}

bool UpperBoundedLinearConstraint::InitializeRhs(Coefficient rhs,
                                                 int trail_index,
                                                 Coefficient* slack,
                                                 Trail* trail,
                                                 std::vector<Literal>* conflict) {
  rhs_ = rhs;

  // Compute the current_rhs from the assigned variables with a trail index
  // smaller than the given trail_index.
  Coefficient current_rhs = rhs;
  int literal_index = 0;
  int coeff_index = 0;
  for (Literal literal : literals_) {
    if (trail->Assignment().IsLiteralTrue(literal) &&
        trail->Info(literal.Variable()).trail_index < trail_index) {
      current_rhs -= coeffs_[coeff_index];
    }
    ++literal_index;
    if (literal_index == starts_[coeff_index + 1]) {
      ++coeff_index;
    }
  }

  // Initial propagation.
  index_ = coeffs_.size() - 1;
  already_propagated_end_ = literals_.size();
  Update(current_rhs, slack);
  return *slack < 0 ? Propagate(trail_index, slack, trail, conflict) : true;
}

bool UpperBoundedLinearConstraint::Propagate(int trail_index,
                                             Coefficient* slack, Trail* trail,
                                             std::vector<Literal>* conflict) {
  DCHECK_LT(*slack, 0);
  const Coefficient current_rhs = GetCurrentRhsFromSlack(*slack);
  while (index_ >= 0 && coeffs_[index_] > current_rhs) --index_;

  // Check propagation.
  VariableIndex first_propagated_variable(-1);
  for (int i = starts_[index_ + 1]; i < already_propagated_end_; ++i) {
    if (trail->Assignment().IsLiteralFalse(literals_[i])) continue;
    if (trail->Assignment().IsLiteralTrue(literals_[i])) {
      if (trail->Info(literals_[i].Variable()).trail_index > trail_index) {
        // Conflict.
        FillReason(*trail, trail_index, literals_[i].Variable(), conflict);
        conflict->push_back(literals_[i].Negated());
        Update(current_rhs, slack);
        return false;
      }
    } else {
      // Propagation.
      if (first_propagated_variable < 0) {
        trail->EnqueueWithPbReason(literals_[i].Negated(), trail_index, this);
        first_propagated_variable = literals_[i].Variable();
      } else {
        // Note that the reason for first_propagated_variable is always a
        // valid reason for literals_[i].Variable() because we process the
        // variable in increasing coefficient order.
        trail->EnqueueWithSameReasonAs(literals_[i].Negated(),
                                       first_propagated_variable);
      }
    }
  }
  Update(current_rhs, slack);
  return *slack >= 0;
}

void UpperBoundedLinearConstraint::FillReason(const Trail& trail,
                                              int source_trail_index,
                                              VariableIndex propagated_variable,
                                              std::vector<Literal>* reason) {
  // Optimization for an "at most one" constraint.
  if (rhs_ == 1) {
    reason->clear();
    reason->push_back(trail[source_trail_index].Negated());
    return;
  }

  // This is needed for unsat proof.
  const bool include_level_zero = trail.NeedFixedLiteralsInReason();

  // Optimization: This will be set to the index of the last literal in the
  // reason, that is the one with smallest indices.
  int last_i = 0;
  int last_coeff_index = 0;

  // Compute the initial reason which is formed by all the literals of the
  // constraint that were assigned to true at the time of the propagation.
  // We remove literals with a level of 0 since they are not needed.
  // We also compute the current_rhs at the time.
  reason->clear();
  Coefficient current_rhs = rhs_;
  Coefficient propagated_variable_coefficient(0);
  int coeff_index = coeffs_.size() - 1;
  for (int i = literals_.size() - 1; i >= 0; --i) {
    const Literal literal = literals_[i];
    if (literal.Variable() == propagated_variable) {
      propagated_variable_coefficient = coeffs_[coeff_index];
    } else {
      if (trail.Assignment().IsLiteralTrue(literal) &&
          trail.Info(literal.Variable()).trail_index <= source_trail_index) {
        if (include_level_zero || trail.Info(literal.Variable()).level != 0) {
          reason->push_back(literal.Negated());
          last_i = i;
          last_coeff_index = coeff_index;
        }
        current_rhs -= coeffs_[coeff_index];
      }
    }
    if (i == starts_[coeff_index]) {
      --coeff_index;
    }
  }
  DCHECK_GT(propagated_variable_coefficient, current_rhs);
  DCHECK_GE(propagated_variable_coefficient, 0);

  // In both cases, we can't minimize the reason further.
  if (reason->size() <= 1 || coeffs_.size() == 1) return;

  Coefficient limit = propagated_variable_coefficient - current_rhs;
  DCHECK_GE(limit, 1);

  // Remove literals with small coefficients from the reason as long as the
  // limit is still stricly positive.
  coeff_index = last_coeff_index;
  if (coeffs_[coeff_index] >= limit) return;
  for (int i = last_i; i < literals_.size(); ++i) {
    const Literal literal = literals_[i];
    if (i == starts_[coeff_index + 1]) {
      ++coeff_index;
      if (coeffs_[coeff_index] >= limit) break;
    }
    if (literal.Negated() != reason->back()) continue;
    limit -= coeffs_[coeff_index];
    reason->pop_back();
    if (coeffs_[coeff_index] >= limit) break;
  }
  DCHECK(!reason->empty());
  DCHECK_GE(limit, 1);
}

void UpperBoundedLinearConstraint::Untrail(Coefficient* slack) {
  const Coefficient current_rhs = GetCurrentRhsFromSlack(*slack);
  while (index_ + 1 < coeffs_.size() && coeffs_[index_ + 1] <= current_rhs)
    ++index_;
  Update(current_rhs, slack);
}

// TODO(user): This is relatively slow. Take the "transpose" all at once, and
// maybe put small constraints first on the to_update_ lists.
bool PbConstraints::AddConstraint(const std::vector<LiteralWithCoeff>& cst,
                                  Coefficient rhs, ResolutionNode* node) {
  SCOPED_TIME_STAT(&stats_);
  DCHECK(!cst.empty());
  DCHECK(std::is_sorted(cst.begin(), cst.end(), CoeffComparator));

  // Optimization if the constraint terms are the same as the one of the last
  // added constraint.
  if (!constraints_.empty() && constraints_.back().HasIdenticalTerms(cst)) {
    if (rhs < constraints_.back().Rhs()) {
      // The new constraint is tighther, so we also replace the ResolutionNode.
      // TODO(user): The old one could be unlocked at this point.
      constraints_.back().ChangeResolutionNode(node);
      return constraints_.back().InitializeRhs(rhs, propagation_trail_index_,
                                               &slacks_.back(), trail_,
                                               &conflict_scratchpad_);
    } else {
      // The constraint is redundant, so there is nothing to do.
      return true;
    }
  }

  const ConstraintIndex cst_index(constraints_.size());
  constraints_.emplace_back(UpperBoundedLinearConstraint(cst, node));
  slacks_.push_back(Coefficient(0));
  if (!constraints_.back().InitializeRhs(rhs, propagation_trail_index_,
                                         &slacks_.back(), trail_,
                                         &conflict_scratchpad_)) {
    return false;
  }
  for (LiteralWithCoeff term : cst) {
    to_update_[term.literal.Index()]
        .push_back(ConstraintIndexWithCoeff(cst_index, term.coefficient));
  }
  return true;
}

bool PbConstraints::PropagateNext() {
  SCOPED_TIME_STAT(&stats_);
  DCHECK(PropagationNeeded());
  const int order = propagation_trail_index_;
  const Literal true_literal = (*trail_)[propagation_trail_index_];
  ++propagation_trail_index_;

  // We need to upate ALL slack, otherwise the Untrail() will not be
  // synchronized.
  //
  // TODO(user): An alternative that sound slightly more efficient is to store
  // an index for this special case so that Untrail() know what to do.
  bool conflict = false;
  num_slack_updates_ += to_update_[true_literal.Index()].size();
  for (ConstraintIndexWithCoeff& update : to_update_[true_literal.Index()]) {
    const Coefficient slack = slacks_[update.index] - update.coefficient;
    slacks_[update.index] = slack;
    if (slack < 0 && !conflict) {
      update.need_untrail_inspection = true;
      ++num_constraint_lookups_;
      if (!constraints_[update.index.value()].Propagate(
               order, &slacks_[update.index], trail_, &conflict_scratchpad_)) {
        trail_->SetFailingClause(ClauseRef(conflict_scratchpad_));
        trail_->SetFailingResolutionNode(
            constraints_[update.index.value()].ResolutionNodePointer());
        conflict = true;
      }
    }
  }
  return !conflict;
}

void PbConstraints::Untrail(int trail_index) {
  SCOPED_TIME_STAT(&stats_);
  to_untrail_.ClearAndResize(ConstraintIndex(constraints_.size()));
  while (propagation_trail_index_ > trail_index) {
    --propagation_trail_index_;
    const Literal literal = (*trail_)[propagation_trail_index_];
    for (ConstraintIndexWithCoeff& update : to_update_[literal.Index()]) {
      slacks_[update.index] += update.coefficient;

      // Only the constraints which where inspected during Propagate() need
      // inspection during Untrail().
      if (update.need_untrail_inspection) {
        update.need_untrail_inspection = false;
        to_untrail_.Set(update.index);
      }
    }
  }
  for (ConstraintIndex cst_index : to_untrail_.PositionsSetAtLeastOnce()) {
    constraints_[cst_index.value()].Untrail(&(slacks_[cst_index]));
  }
}

}  // namespace sat
}  // namespace operations_research
