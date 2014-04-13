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
#ifndef OR_TOOLS_SAT_SYMMETRY_H_
#define OR_TOOLS_SAT_SYMMETRY_H_

#include "sat/sat_base.h"
#include "algorithms/sparse_permutation.h"
#include "util/stats.h"

namespace operations_research {
namespace sat {

// This class implements more or less the strategy described in the paper:
// Devriendt J., Bogaerts B., De Cat B., Denecker M., Mears C. "Symmetry
// propagation: Improved Dynamic Symmetry Breaking in SAT", 2012,
// IEEE 24th International Conference on Tools with Artificial Intelligence.
//
// Basically, each time a literal is propagated, this class tries to detect
// if another literal could also be propagated by symmetry. Note that this uses
// an heuristic in order to be efficient and that it is not exhaustive in the
// sense that it doesn't detect all possible propagations.
//
// Algorithm details:
//
// Given the current solver trail (i.e. the assigned literals and their
// assignment order) the idea is to compute (as efficiently as possible) for
// each permutation added to this class what is called the first (under the
// trail assignment order) non-symmetric literal. A literal 'l' is said to be
// non-symmetric under a given assignement and for a given permutation 'p' if
// 'l' is assigned to true but not 'p(l)'.
//
// If a first non-symmetric literal 'l' for a permutation 'p' is not a decision,
// then:
// - Because it is not a decision, 'l' has been implied by a reason formed by
//   literals assigned to true at lower trail indices.
// - Because this is the first non-symmetric literal for 'p', the permuted
//   reason only contains literal that are also assigned to true.
// - Because of this, 'p(l)' is also implied by the current assignment.
//   Of course, this assume that p is a symmetry of the full problem.
//   Note that if it is already assigned to false, then we have a conflict.
//
// TODO(user): Implements the optimizations mentioned in the paper?
// TODO(user): Instrument and see if the code can be optimized.
class SymmetryPropagator {
 public:
  explicit SymmetryPropagator(Trail* trail);  // No ownership taken.
  ~SymmetryPropagator();

  // Changes the number of variables. This must be higher than:
  // - Any variable touched by a symmetry about to be added or already added.
  // - Any variable assigned by the trail when calling PropagateNext().
  void Resize(int num_variables);

  // Adds a new permutation to this symmetry propagator. The ownership is
  // transfered. This must be an integer permutation such that:
  // - Its domain is [0, 2 * num_variables) and corresponds to the index
  //   representation of the literals over num_variables variables.
  // - It must be compatible with the negation, for any literal l; not(p(l))
  //   must be the same as p(not(l)), where p(x) represents the image of x by
  //   the permutation.
  //
  // Remark: Any permutation which is a symmetry of the main SAT problem can be
  // added here. However, since the number of permutations is usually not
  // manageable, a good alternative is to only add the generators of the
  // permutation group. It is also important to add permutations with a support
  // as small as possible.
  //
  // TODO(user): Currently this can only be called before PropagateNext() is
  // called (DCHECKed). Not sure if we need more incrementality though.
  void AddSymmetry(std::unique_ptr<SparsePermutation> permutation);

  // If some literals enqueued on the trail haven't been processed by this class
  // then PropagationNeeded() will returns true. In this case, it is possible to
  // call PropagateNext() to process them.
  //
  // PropagateNext() will returns false if a conflict in the assignment is
  // detected. Note that it will also abort early if some new literals are
  // propagated, so in order to propagate everything, you need a construct like
  // while (PropagationNeeded()) PropagateNext();
  bool PropagationNeeded() const;
  bool PropagateNext();

  // Backtracks to the state where all the literals with trail index greater or
  // equal to the given one are assumed to be unassigned.
  void Untrail(int trail_index);

  // Functions to get the clause representing the last conflict. One must first
  // query the reason of the assignment of the variable
  // VariableAtTheSourceOfLastConflict() and then call LastConflict() with this
  // reason.
  VariableIndex VariableAtTheSourceOfLastConflict() const;
  const std::vector<Literal>& LastConflict(ClauseRef initial_reason) const;

  // Permutes a list of literals from input into output using the permutation
  // with given index. This uses tmp_literal_mapping_ and has a complexity in
  // O(permutation_support + input_size).
  void Permute(int index, ClauseRef input, std::vector<Literal>* output) const;

 private:
  // The solver trail that contains the variables assignements and all the
  // assignment info. propagation_trail_index_ is the index of the first
  // assigned variable from the trail that is not yet processed by this class.
  Trail* trail_;
  int propagation_trail_index_;

  // The permutations.
  // The index of a permutation is its position in this vector.
  std::vector<std::unique_ptr<SparsePermutation>> permutations_;

  // Reverse mapping (source literal) -> list of (permutation_index, image).
  struct ImageInfo {
    ImageInfo(int p, Literal i) : permutation_index(p), image(i) {}

    int permutation_index;
    Literal image;
  };
  ITIVector<LiteralIndex, std::vector<ImageInfo>> images_;

  // For each permutation p, we maintain the list of all assigned literals
  // affected by p whose trail index is < propagation_trail_index_; sorted by
  // trail index.  Next to each such literal, we also store:
  struct AssignedLiteralInfo {
    AssignedLiteralInfo(Literal l, Literal i, int index)
        : literal(l), image(i), first_non_symmetric_info_index_so_far(index) {}

    // The literal in question (assigned to true and in the support of p).
    Literal literal;

    // The image by p of the literal above.
    Literal image;

    // Previous AssignedLiteralInfos are considered 'symmetric' iff both their
    // 'literal' and 'image' were assigned to true at the time the current
    // AssignedLiteralInfo's literal was assigned (i.e. earlier in the trail).
    int first_non_symmetric_info_index_so_far;
  };
  std::vector<std::vector<AssignedLiteralInfo>> permutation_trails_;

  // Adds an AssignedLiteralInfo to the given permutation trail.
  // Returns false if there is a non-symmetric literal in this trail with its
  // image not already assigned to true by the solver.
  bool Enqueue(Literal literal, Literal image,
               std::vector<AssignedLiteralInfo>* permutation_trail);

  // The identity permutation over all the literals.
  // This is temporary modified to encode a sparse permutation and then always
  // restored to the identity.
  mutable ITIVector<LiteralIndex, Literal> tmp_literal_mapping_;

  // Temporary data needed to compute the last conflict.
  int conflict_permutation_index_;
  Literal conflict_source_reason_;
  Literal conflict_literal_;
  mutable std::vector<Literal> conflict_scratchpad_;

  mutable StatsGroup stats_;
  int num_propagations_;
  int num_conflicts_;
  DISALLOW_COPY_AND_ASSIGN(SymmetryPropagator);
};

}  // namespace sat
}  // namespace operations_research

#endif  // OR_TOOLS_SAT_SYMMETRY_H_
