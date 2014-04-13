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
// This file contains the solver internal representation of the clauses and the
// classes used for their propagation.

#ifndef OR_TOOLS_SAT_CLAUSE_H_
#define OR_TOOLS_SAT_CLAUSE_H_

#include "base/unique_ptr.h"
#include <queue>
#include <string>
#include <vector>

#include "base/integral_types.h"
#include "base/logging.h"
#include "base/stringprintf.h"
#include "base/timer.h"
#include "base/int_type_indexed_vector.h"
#include "base/int_type.h"
#include "sat/sat_base.h"
#include "sat/sat_parameters.pb.h"
#include "util/bitset.h"
#include "util/stats.h"

namespace operations_research {
namespace sat {

// Forward declarations.
// TODO(user): This cyclic dependency can be relatively easily removed.
class LiteralWatchers;

// Variable information. This is updated each time we attach/detach a clause.
struct VariableInfo {
  VariableInfo()
      : num_positive_clauses(0),
        num_negative_clauses(0),
        num_appearances(0),
        weighted_num_appearances(0.0) {}

  int num_positive_clauses;
  int num_negative_clauses;
  int num_appearances;
  double weighted_num_appearances;
};

// This is how the SatSolver store a clause. A clause is just a disjunction of
// literals. In many places, we just use std::vector<literal> to encode one. However,
// the solver needs to keep a few extra fields attached to each clause.
class SatClause {
 public:
  // Creates a sat clause. There must be at least 2 literals.
  // Smaller clause are treated separatly and never constructed.
  enum ClauseType {
    PROBLEM_CLAUSE,
    LEARNED_CLAUSE,
  };
  static SatClause* Create(const std::vector<Literal>& literals, ClauseType type,
                           ResolutionNode* node);

  // Number of literals in the clause.
  int Size() const { return size_; }

  // Allows for range based iteration: for (Literal literal : clause) {}.
  const Literal* const begin() const { return &(literals_[0]); }
  const Literal* const end() const { return &(literals_[size_]); }
  Literal* literals() { return &(literals_[0]); }

  // Returns the first and second literals. These are always the watched
  // literals if the clause is attached in the LiteralWatchers.
  Literal FirstLiteral() const { return literals_[0]; }
  Literal SecondLiteral() const { return literals_[1]; }

  // Returns the literal that was propagated to true. This only works for a
  // clause that just propagated this literal. Otherwise, this will just returns
  // a literal of the clause.
  Literal PropagatedLiteral() const { return literals_[0]; }

  // Returns the reason for the last unit propagation of this clause. The
  // preconditions are the same as for PropagatedLiteral().
  ClauseRef PropagationReason() const {
    // Note that we don't need to include the propagated literal.
    return ClauseRef(&(literals_[1]), end());
  }

  // Removes literals that are fixed. This should only be called at level 0
  // where a literal is fixed iff it is assigned. Aborts and returns true if
  // they are not all false.
  bool RemoveFixedLiteralsAndTestIfTrue(const VariablesAssignment& assignment,
                                        std::vector<Literal>* removed_literals);

  // True if the clause is learned.
  bool IsLearned() const { return is_learned_; }

  // Returns true if the clause is satisfied for the given assignment. Note that
  // the assignment may be partial, so false does not mean that the clause can't
  // be satisfied by completing the assignment.
  bool IsSatisfied(const VariablesAssignment& assignment) const;

  // Sorts the literals of the clause depending on the given parameters and
  // statistics. Do not call this on an attached clause.
  void SortLiterals(const ITIVector<VariableIndex, VariableInfo>& statistics,
                    const SatParameters& parameters);

  // Sets up the 2-watchers data structure. It selects two non-false literals
  // and attaches the clause to the event: one of the watched literals become
  // false. It returns false if the clause only contains literals assigned to
  // false. If only one literals is not false, it propagates it to true if it
  // is not already assigned.
  bool AttachAndEnqueuePotentialUnitPropagation(Trail* trail,
                                                LiteralWatchers* demons);

  // Modify and get the clause activity.
  void IncreaseActivity(double increase) { activity_ += increase; }
  void MultiplyActivity(double factor) { activity_ *= factor; }
  double Activity() const { return activity_; }

  // Set and get the clause LBD (Literal Blocks Distance). The LBD is not
  // computed here. See ComputeClauseLbd() in SatSolver.
  void SetLbd(int value) { lbd_ = value; }
  int Lbd() const { return lbd_; }

  // Returns true if the clause is attached to a LiteralWatchers.
  bool IsAttached() const { return is_attached_; }

  // Marks the clause so that the next call to CleanUpWatchers() can identify it
  // and actually detach it.
  void LazyDetach() { is_attached_ = false; }

  // Returns the node of the resolution DAG associated to this clause.
  // This will always be nullptr if the parameter unsat_proof() is false.
  ResolutionNode* ResolutionNodePointer() const { return resolution_node_; }
  void ChangeResolutionNode(ResolutionNode* node) { resolution_node_ = node; }

  std::string DebugString() const;

 private:
  // The data is packed so that only 16 bytes are used for these fields.
  // Note that the max lbd is the maximum depth of the search tree (decision
  // levels), so it should fit easily in 30 bits. Note that we can also upper
  // bound it without hurting too much the clause cleaning heuristic.
  bool is_learned_ : 1;
  bool is_attached_ : 1;
  int lbd_ : 30;
  int size_ : 32;
  double activity_;

  // This is only needed when the parameter unsat_proof() is true.
  // TODO(user): It is possible to use less memory when this is not the case
  // by some tweaks in Create() and in the way we access it. Note that not
  // storing this seems to gain about 2% in speed overall.
  ResolutionNode* resolution_node_;

  // This class store the literals inline, and literals_ mark the starts of the
  // variable length portion.
  Literal literals_[0];

  DISALLOW_COPY_AND_ASSIGN(SatClause);
};

// Stores the 2-watched literals data structure.  See
// http://www.cs.berkeley.edu/~necula/autded/lecture24-sat.pdf for
// detail.
class LiteralWatchers {
 public:
  LiteralWatchers();
  ~LiteralWatchers();

  // Resizes the data structure.
  void Resize(int num_variables);

  // Attaches the given clause. This eventually propagates a literal which is
  // enqueued on the trail. Returns false if a contradiction was encountered.
  bool AttachAndPropagate(SatClause* clause, Trail* trail);

  // Attaches the given clause to the event: the given literal becomes false.
  // The blocking_literal can be any literal from the clause, it is used to
  // speed up PropagateOnFalse() by skipping the clause if it is true.
  void AttachOnFalse(Literal literal, Literal blocking_literal,
                     SatClause* clause);

  // Lazily detach the given clause. The deletion will actually occur when
  // CleanUpWatchers() is called. The later needs to be called before any other
  // function in this class can be called. This is DCHECKed.
  void LazyDetach(SatClause* clause);
  void CleanUpWatchers();

  // Launches all propagation when the given literal becomes false.
  // Returns false if a contradiction was encountered.
  bool PropagateOnFalse(Literal false_literal, Trail* trail);

  // Total number of clauses inspected during calls to PropagateOnFalse().
  int64 num_inspected_clauses() const { return num_inspected_clauses_; }

  // Number of clauses currently watched.
  int64 num_watched_clauses() const { return num_watched_clauses_; }

  // Returns some statistics on the number of appearance of this variable in
  // all the attached clauses.
  const VariableInfo& VariableStatistic(VariableIndex var) const {
    return statistics_[var];
  }

  // Parameters management.
  void SetParameters(const SatParameters& parameters) {
    parameters_ = parameters;
  }

 private:
  // Updates statistics_ for the literals in the given clause. added indicates
  // if we are adding the clause or deleting it.
  void UpdateStatistics(const SatClause& clause, bool added);

  // Contains, for each literal, the list of clauses that need to be inspected
  // when the corresponding literal becomes false.
  struct Watcher {
    Watcher() {}
    Watcher(SatClause* c, Literal b) : clause(c), blocking_literal(b) {}
    SatClause* clause;
    Literal blocking_literal;
  };
  ITIVector<LiteralIndex, std::vector<Watcher> > watchers_on_false_;

  // Indicates if the corresponding watchers_on_false_ list need to be
  // cleaned. The boolean is_clean_ is just used in DCHECKs.
  ITIVector<LiteralIndex, bool> needs_cleaning_;
  bool is_clean_;

  ITIVector<VariableIndex, VariableInfo> statistics_;
  SatParameters parameters_;
  int64 num_inspected_clauses_;
  int64 num_watched_clauses_;
  mutable StatsGroup stats_;
  DISALLOW_COPY_AND_ASSIGN(LiteralWatchers);
};

// Special class to store and propagate clauses of size 2 (i.e. implication).
// Such clauses are never deleted.
//
// TODO(user): All the variables in a strongly connected component are
// equivalent and can be thus merged as one. This is relatively cheap to compute
// from time to time (linear complexity). We will also get contradiction (a <=>
// not a) this way.
//
// TODO(user): An implication (a => not a) implies that a is false. I am not
// sure it is worth detecting that because if the solver assign a to true, it
// will learn that right away. I don't think we can do it faster.
//
// TODO(user): The implication graph can be pruned. This is called the
// transitive reduction of a graph. For instance If a => {b,c} and b => {c},
// then there is no need to store a => {c}. The transitive reduction is unique
// on an acyclic graph. Computing it will allow for a faster propagation and
// memory reduction. It is however not cheap. Maybe simple lazy heuristics to
// remove redundant arcs are better. Note that all the learned clauses we add
// will never be redundant (but they could introduce cycles).
//
// TODO(user): Add a preprocessor to remove duplicates in the implication lists.
// Note that all the learned clauses we had will never create duplicates.
//
// References for most of the above TODO and more:
// - Brafman RI, "A simplifier for propositional formulas with many binary
//   clauses", IEEE Trans Syst Man Cybern B Cybern. 2004 Feb;34(1):52-9.
//   http://citeseerx.ist.psu.edu/viewdoc/summary?doi=10.1.1.28.4911
// - Marijn J. H. Heule, Matti Järvisalo, Armin Biere, "Efficient CNF
//   Simplification Based on Binary Implication Graphs", Theory and Applications
//   of Satisfiability Testing - SAT 2011, Lecture Notes in Computer Science
//   Volume 6695, 2011, pp 201-215
//   http://www.cs.helsinki.fi/u/mjarvisa/papers/heule-jarvisalo-biere.sat11.pdf
class BinaryImplicationGraph {
 public:
  BinaryImplicationGraph()
      : num_implications_(0),
        num_propagations_(0),
        num_minimization_(0),
        num_literals_removed_(0),
        num_redundant_implications_(0),
        stats_("BinaryImplicationGraph") {}
  ~BinaryImplicationGraph() {
    IF_STATS_ENABLED({
      LOG(INFO) << stats_.StatString();
      LOG(INFO) << "num_redundant_implications " << num_redundant_implications_;
    });
  }

  // Resizes the data structure.
  void Resize(int num_variables);

  // Adds the binary clause (a OR b), which is the same as (not a => b).
  // Note that it is also equivalent to (not b => a).
  void AddBinaryClause(Literal a, Literal b);

  // Same as AddBinaryClause() but enqueues a possible unit propagation.
  void AddBinaryConflict(Literal a, Literal b, Trail* trail);

  // Propagates all the direct implications of the given literal becoming true.
  // Returns false if a conflict was encountered, in which case
  // trail->SetFailingClause() will be called with the correct size 2 clause.
  // This calls trail->Enqueue() on the newly assigned literals.
  bool PropagateOnTrue(Literal true_literal, Trail* trail);

  // Uses the binary implication graph to minimize the given conflict by
  // removing literals that implies others. The idea is that if a and b are two
  // literals from the given conflict and a => b (which is the same as not(b) =>
  // not(a)) then a is redundant and can be removed.
  //
  // Note that removing as many literals as possible is too time consuming, so
  // we use different heuristics/algorithms to do this minimization.
  // See the binary_minimization_algorithm SAT parameter and the .cc for more
  // details about the different algorithms.
  void MinimizeConflictWithReachability(std::vector<Literal>* c);
  void MinimizeConflictExperimental(const Trail& trail, std::vector<Literal>* c);
  void MinimizeConflictFirst(const Trail& trail, std::vector<Literal>* c,
                             SparseBitset<VariableIndex>* marked);

  // This must only be called at decision level 0 after all the possible
  // propagations. It:
  // - Removes the variable at true from the implications lists.
  // - Frees the propagation list of the assigned literals.
  void RemoveFixedVariables(const VariablesAssignment& assigment);

  // Number of literal propagated by this class (including conflicts).
  int64 num_propagations() const { return num_propagations_; }

  // MinimizeClause() stats.
  int64 num_minimization() const { return num_minimization_; }
  int64 num_literals_removed() const { return num_literals_removed_; }

  // Returns the number of current implications.
  int64 NumberOfImplications() const { return num_implications_; }

 private:
  // Remove any literal whose negation is marked (except the first one).
  void RemoveRedundantLiterals(std::vector<Literal>* conflict);

  // This is indexed by the Index() of a literal. Each list stores the
  // literals that are implied if the index literal becomes true.
  ITIVector<LiteralIndex, std::vector<Literal> > implications_;
  int64 num_implications_;

  // Holds the last conflicting binary clause.
  Literal temporary_clause_[2];

  // Some stats.
  int64 num_propagations_;
  int64 num_minimization_;
  int64 num_literals_removed_;
  int64 num_redundant_implications_;

  // Bitset used by MinimizeClause().
  // TODO(user): use the same one as the one used in the classic minimization
  // because they are already initialized. Moreover they contains more
  // information.
  SparseBitset<LiteralIndex> is_marked_;
  SparseBitset<LiteralIndex> is_removed_;

  // Temporary stack used by MinimizeClauseWithReachability().
  std::vector<Literal> dfs_stack_;

  mutable StatsGroup stats_;
  DISALLOW_COPY_AND_ASSIGN(BinaryImplicationGraph);
};

}  // namespace sat
}  // namespace operations_research

#endif  // OR_TOOLS_SAT_CLAUSE_H_
