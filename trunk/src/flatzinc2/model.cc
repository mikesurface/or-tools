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
#include "base/hash.h"
#include <iostream>  // NOLINT

#include "base/concise_iterator.h"
#include "base/map_util.h"
#include "base/stl_util.h"
#include "flatzinc2/model.h"
#include "flatzinc2/parser.h"

DEFINE_bool(fz_logging, false,
            "Print logging information from the flatzinc interpreter.");

namespace operations_research {
// ----- FzDomain -----

FzDomain FzDomain::IntegerList(const std::vector<int64>& values) {
  FzDomain result;
  result.is_interval = false;
  result.values = values;
  return result;
}

FzDomain FzDomain::AllInt64() {
  FzDomain result;
  result.is_interval = true;
  return result;
}

FzDomain FzDomain::Singleton(int64 value) {
  FzDomain result;
  result.is_interval = false;
  result.values.push_back(value);
  return result;
}

FzDomain FzDomain::Interval(int64 included_min, int64 included_max) {
  FzDomain result;
  result.is_interval = true;
  result.values.push_back(included_min);
  result.values.push_back(included_max);
  return result;
}

void FzDomain::IntersectWith(const FzDomain& other) {
  if (other.is_interval) {
    if (!other.values.empty()) {
      ReduceDomain(other.values[0], other.values[1]);
      return;
    }
    if (is_interval) {
      is_interval = false;  // Other is not an interval.
      if (values.empty()) {
        values = other.values;
      } else {
        const int64 imin = values[0];
        const int64 imax = values[1];
        values = other.values;
        ReduceDomain(imin, imax);
      }
      return;
    }
  }
  // now deal with the intersection of two lists of values
  // TODO(user): Investigate faster code for small arrays.
  hash_set<int64> other_values(other.values.begin(), other.values.end());
  std::vector<int64> new_values;
  new_values.reserve(std::min(values.size(), other.values.size()));
  for (int i = 0; i < values.size(); ++i) {
    const int64 val = values[i];
    if (ContainsKey(other_values, val)) {
      new_values.push_back(val);
    }
  }
  values.swap(new_values);
}

void FzDomain::ReduceDomain(int64 imin, int64 imax) {
  if (is_interval) {
    if (values.empty()) {
      values.push_back(imin);
      values.push_back(imax);
    } else {
      values[0] = std::max(values[0], imin);
      values[1] = std::min(values[1], imax);
    }
  } else {
    std::vector<int64> new_values;
    new_values.reserve(values.size());
    for (int i = 0; i < values.size(); ++i) {
      const int64 val = values[i];
      if (val >= imin && val <= imax) {
        new_values.push_back(val);
      }
    }
    values.swap(new_values);
  }
}

bool FzDomain::IsSingleton() const {
  return (values.size() == 1 || (values.size() == 2 && values[0] == values[1]));
}

bool FzDomain::Contains(int64 value) const {
  if (is_interval) {
    if (values.empty()) {
      return true;
    } else {
      return value >= values[0] && value <= values[1];
    }
  } else {
    return std::find(values.begin(), values.end(), value) != values.end();
  }
}

std::string FzDomain::DebugString() const {
  if (is_interval) {
    if (values.empty()) {
      return "int";
    } else {
      return StringPrintf("%" GG_LL_FORMAT "d..%" GG_LL_FORMAT "d", values[0],
                          values[1]);
    }
  } else if (values.size() == 1) {
    return StringPrintf("% " GG_LL_FORMAT "d", values.back());
  } else {
    return StringPrintf("[%s]", IntVectorToString(values, ", ").c_str());
  }
}

// ----- FzArgument -----

FzArgument FzArgument::IntegerValue(int64 value) {
  FzArgument result;
  result.type = INT_VALUE;
  result.integer_value = value;
  result.domain = FzDomain::AllInt64();
  result.variable = nullptr;
  return result;
}

FzArgument FzArgument::Domain(const FzDomain& domain) {
  FzArgument result;
  result.type = INT_DOMAIN;
  result.integer_value = 0;
  result.domain = domain;
  result.variable = nullptr;
  return result;
}

FzArgument FzArgument::IntVarRef(FzIntegerVariable* const var) {
  FzArgument result;
  result.type = INT_VAR_REF;
  result.integer_value = 0;
  result.domain = FzDomain::AllInt64();
  result.variable = var;
  return result;
}

FzArgument FzArgument::IntVarRefArray(const std::vector<FzIntegerVariable*>& vars) {
  FzArgument result;
  result.type = INT_VAR_REF_ARRAY;
  result.integer_value = 0;
  result.domain = FzDomain::AllInt64();
  result.variable = nullptr;
  result.variables = vars;
  return result;
}

FzArgument FzArgument::VoidArgument() {
  FzArgument result;
  result.type = VOID_ARGUMENT;
  result.integer_value = 0;
  result.domain = FzDomain::AllInt64();
  result.variable = nullptr;
  return result;
}

std::string FzArgument::DebugString() const {
  switch (type) {
    case INT_VALUE:
      return StringPrintf("% " GG_LL_FORMAT "d", integer_value);
    case INT_DOMAIN:
      return domain.DebugString();
    case INT_VAR_REF:
      return variable->name;
    case INT_VAR_REF_ARRAY: {
      std::string result = "[";
      for (int i = 0; i < variables.size(); ++i) {
        result.append(variables[i]->name);
        result.append(i != variables.size() - 1 ? ", " : "]");
      }
      return result;
    }
    case VOID_ARGUMENT:
      return "VoidArgument";
  }
}

// ----- FzIntegerVariable -----

FzIntegerVariable::FzIntegerVariable(const std::string& name_,
                                     const FzDomain& domain_, bool temporary_)
    : name(name_),
      domain(domain_),
      defining_constraint(nullptr),
      temporary(temporary_) {}

bool FzIntegerVariable::Merge(const std::string& other_name,
                              const FzDomain& other_domain,
                              FzConstraint* const other_constraint,
                              bool other_temporary) {
  if (defining_constraint != nullptr && other_constraint != nullptr) {
    // Both are defined, we cannot merge the two variables.
    return false;
  }
  if (temporary && !other_temporary) {
    temporary = false;
    name = other_name;
  }
  if (defining_constraint == nullptr) {
    defining_constraint = other_constraint;
  }
  domain.IntersectWith(other_domain);
  return true;
}

std::string FzIntegerVariable::DebugString() const {
  if (!domain.is_interval && domain.values.size() == 1) {
    return StringPrintf("% " GG_LL_FORMAT "d", domain.values.back());
  } else {
    return StringPrintf("%s(%s%s%s)", name.c_str(),
                        domain.DebugString().c_str(),
                        temporary ? ", temporary" : "",
                        defining_constraint != nullptr ? ", target_var" : "");
  }
}

// ----- FzConstraint -----

std::string FzConstraint::DebugString() const {
  const std::string strong = strong_propagation ? ", strong propagation" : "";
  const std::string target = target_var != nullptr
                            ? StringPrintf(" => %s", target_var->name.c_str())
                            : "";
  return StringPrintf("%s([%s]%s)%s", type.c_str(),
                      JoinDebugString(arguments, ", ").c_str(), strong.c_str(),
                      target.c_str());
}

// ----- FzAnnotation -----

FzAnnotation FzAnnotation::Empty() {
  FzAnnotation result;
  result.type = ANNOTATION_LIST;
  result.interval_min = 0;
  result.interval_max = 0;
  result.variable = nullptr;
  return result;
}

FzAnnotation FzAnnotation::AnnotationList(const std::vector<FzAnnotation>& list) {
  FzAnnotation result;
  result.type = ANNOTATION_LIST;
  result.interval_min = 0;
  result.interval_max = 0;
  result.annotations = list;
  result.variable = nullptr;
  return result;
}

FzAnnotation FzAnnotation::Identifier(const std::string& id) {
  FzAnnotation result;
  result.type = IDENTIFIER;
  result.interval_min = 0;
  result.interval_max = 0;
  result.id = id;
  result.variable = nullptr;
  return result;
}

FzAnnotation FzAnnotation::FunctionCall(const std::string& id,
                                        const std::vector<FzAnnotation>& args) {
  FzAnnotation result;
  result.type = FUNCTION_CALL;
  result.interval_min = 0;
  result.interval_max = 0;
  result.id = id;
  result.annotations = args;
  result.variable = nullptr;
  return result;
}

FzAnnotation FzAnnotation::Interval(int64 interval_min, int64 interval_max) {
  FzAnnotation result;
  result.type = INTERVAL;
  result.interval_min = interval_min;
  result.interval_max = interval_max;
  result.variable = nullptr;
  return result;
}

FzAnnotation FzAnnotation::Variable(FzIntegerVariable* const var) {
  FzAnnotation result;
  result.type = INT_VAR_REF;
  result.interval_min = 0;
  result.interval_max = 0;
  result.variable = var;
  return result;
}

FzAnnotation FzAnnotation::VariableList(
    const std::vector<FzIntegerVariable*>& vars) {
  FzAnnotation result;
  result.type = INT_VAR_REF_ARRAY;
  result.interval_min = 0;
  result.interval_max = 0;
  result.variable = nullptr;
  result.variables = vars;
  return result;
}

void FzAnnotation::GetAllIntegerVariables(
    std::vector<FzIntegerVariable*>* const vars) const {
  for (int i = 0; i < annotations.size(); ++i) {
    annotations[i].GetAllIntegerVariables(vars);
  }
  if (variable != nullptr) {
    vars->push_back(variable);
  }
  if (!variables.empty()) {
    vars->insert(vars->end(), variables.begin(), variables.end());
  }
}

std::string FzAnnotation::DebugString() const {
  switch (type) {
    case ANNOTATION_LIST: {
      return StringPrintf("[%s]", JoinDebugString(annotations, ", ").c_str());
    }
    case IDENTIFIER: { return id; }
    case FUNCTION_CALL: {
      return StringPrintf("%s(%s)", id.c_str(),
                          JoinDebugString(annotations, ", ").c_str());
    }
    case INTERVAL: {
      return StringPrintf("%" GG_LL_FORMAT "d..%" GG_LL_FORMAT "d",
                          interval_min, interval_max);
    }
    case INT_VAR_REF: { return variable->name; }
    case INT_VAR_REF_ARRAY: {
      std::string result = "[";
      for (int i = 0; i < variables.size(); ++i) {
        result.append(variables[i]->name);
        result.append(i != variables.size() - 1 ? ", " : "]");
      }
      return result;
    }
  }
}

// ----- FzOnSolutionOutput -----

std::string FzOnSolutionOutput::Bounds::DebugString() const {
  return StringPrintf("%" GG_LL_FORMAT "d..%" GG_LL_FORMAT "d", min_value,
                      max_value);
}

FzOnSolutionOutput FzOnSolutionOutput::SingleVariable(
    const std::string& name, FzIntegerVariable* const variable) {
  FzOnSolutionOutput result;
  result.name = name;
  result.variable = variable;
  return result;
}

FzOnSolutionOutput FzOnSolutionOutput::MultiDimensionalArray(
    const std::string& name, const std::vector<Bounds>& bounds,
    const std::vector<FzIntegerVariable*>& flat_variables) {
  FzOnSolutionOutput result;
  result.name = name;
  result.bounds = bounds;
  result.flat_variables = flat_variables;
  return result;
}

FzOnSolutionOutput FzOnSolutionOutput::VoidOutput() {
  FzOnSolutionOutput result;
  result.variable = nullptr;
  return result;
}

std::string FzOnSolutionOutput::DebugString() const {
  if (variable != nullptr) {
    return StringPrintf("output_var(%s)", variable->name.c_str());
  } else {
    return StringPrintf("output_array([%s] [%s])",
                        JoinDebugString(bounds, ", ").c_str(),
                        JoinNameFieldPtr(flat_variables, ", ").c_str());
  }
}

// ----- FzModel -----

FzModel::~FzModel() {
  STLDeleteElements(&variables_);
  STLDeleteElements(&constraints_);
}

FzIntegerVariable* FzModel::AddVariable(const std::string& name,
                                        const FzDomain& domain, bool defined) {
  FzIntegerVariable* const var = new FzIntegerVariable(name, domain, defined);
  variables_.push_back(var);
  return var;
}

void FzModel::AddConstraint(const std::string& id,
                            const std::vector<FzArgument>& arguments, bool is_domain,
                            FzIntegerVariable* const defines) {
  FzConstraint* const constraint =
      new FzConstraint(id, arguments, is_domain, defines);
  constraints_.push_back(constraint);
  if (defines != nullptr) {
    defines->defining_constraint = constraint;
  }
}

void FzModel::AddOutput(const FzOnSolutionOutput& output) {
  output_.push_back(output);
}

void FzModel::Satisfy(const std::vector<FzAnnotation>& search_annotations) {
  objective_ = nullptr;
  search_annotations_ = search_annotations;
}

void FzModel::Minimize(FzIntegerVariable* const obj,
                       const std::vector<FzAnnotation>& search_annotations) {
  objective_ = obj;
  maximize_ = false;
  search_annotations_ = search_annotations;
}

void FzModel::Maximize(FzIntegerVariable* const obj,
                       const std::vector<FzAnnotation>& search_annotations) {
  objective_ = obj;
  maximize_ = true;
  search_annotations_ = search_annotations;
}

std::string FzModel::DebugString() const {
  std::string output = StringPrintf("Model %s\nVariables\n", name_.c_str());
  for (int i = 0; i < variables_.size(); ++i) {
    StringAppendF(&output, "  %s\n", variables_[i]->DebugString().c_str());
  }
  output.append("Constraints\n");
  for (int i = 0; i < constraints_.size(); ++i) {
    if (constraints_[i] != nullptr) {
      StringAppendF(&output, "  %s\n", constraints_[i]->DebugString().c_str());
    }
  }
  if (objective_ != nullptr) {
    StringAppendF(&output, "%s %s\n  %s\n", maximize_ ? "Maximize" : "Minimize",
                  objective_->name.c_str(),
                  JoinDebugString(search_annotations_, ", ").c_str());
  } else {
    StringAppendF(&output, "Satisfy\n  %s\n",
                  JoinDebugString(search_annotations_, ", ").c_str());
  }
  output.append("Output\n");
  for (int i = 0; i < output_.size(); ++i) {
    StringAppendF(&output, "  %s\n", output_[i].DebugString().c_str());
  }

  return output;
}

// ----- Model statistics -----

void FzModelStatistics::PrintStatistics() {
  BuildStatistics();
  FZLOG << "Model statistics" << std::endl;
  BuildStatistics();
  for (ConstIter<hash_map<const std::string, std::vector<FzConstraint*>>> it(
           constraints_per_type_);
       !it.at_end(); ++it) {
    FZLOG << "  - " << it->first << ": " << it->second.size() << std::endl;
  }
  if (model_.objective() == nullptr) {
    FZLOG << "  - Satisfaction problem" << std::endl;
  } else {
    FZLOG << "  - " << (model_.maximize() ? "Maximization" : "Minimization")
          << " problem" << std::endl;
  }
}

void FzModelStatistics::BuildStatistics() {
  constraints_per_type_.clear();
  constraints_per_variables_.clear();
  for (int i = 0; i < model_.constraints().size(); ++i) {
    FzConstraint* const ct = model_.constraints()[i];
    if (ct != nullptr) {
      constraints_per_type_[ct->type].push_back(ct);
      hash_set<const FzIntegerVariable*> marked;
      for (int j = 0; j < ct->arguments.size(); ++j) {
        const FzArgument& arg = ct->arguments[j];
        if (arg.type == FzArgument::INT_VAR_REF) {
          marked.insert(arg.variable);
        } else if (arg.type == FzArgument::INT_VAR_REF_ARRAY) {
          for (int k = 0; k < arg.variables.size(); ++k) {
            marked.insert(arg.variables[k]);
          }
        }
      }
      for (ConstIter<hash_set<const FzIntegerVariable*>> it(marked);
           !it.at_end(); ++it) {
        constraints_per_variables_[*it].push_back(ct);
      }
    }
  }
}
}  // namespace operations_research
