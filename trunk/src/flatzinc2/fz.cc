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
// This is the skeleton for the official flatzinc interpreter.  Much
// of the funcionnalities are fixed (name of parameters, format of the
// input): see http://www.minizinc.org/downloads/doc-1.6/flatzinc-spec.pdf

#include <iostream>  // NOLINT
#include <string>
#include <vector>

#include "base/commandlineflags.h"
#include "base/stringprintf.h"
#include "base/integral_types.h"
#include "base/logging.h"
#include "flatzinc2/model.h"
#include "flatzinc2/parser.h"
#include "flatzinc2/presolve.h"
#include "flatzinc2/search.h"
#include "flatzinc2/solver.h"

DEFINE_int32(log_period, 10000000, "Search log period");
DEFINE_bool(all, false, "Search for all solutions");
DEFINE_bool(free, false, "Ignore search annotations");
DEFINE_int32(num_solutions, 0, "Number of solution to search for");
DEFINE_int32(time_limit, 0, "time limit in ms");
DEFINE_int32(workers, 0, "Number of workers");
DEFINE_bool(use_impact, false, "Use impact based search");
DEFINE_double(restart_log_size, -1, "Restart log size for impact search");
DEFINE_int32(luby_restart, -1, "Luby restart factor, <= 0 = no luby");
DEFINE_int32(heuristic_period, 100, "Period to call heuristics in free search");
DEFINE_bool(verbose_impact, false, "Verbose impact");
DEFINE_bool(verbose_mt, false, "Verbose Multi-Thread");

DECLARE_bool(log_prefix);
DECLARE_bool(fz_logging);

namespace operations_research {
void SequentialRun(const std::string& filename) {
  FzSolverParameters parameters;
  parameters.all_solutions = FLAGS_all;
  parameters.free_search = FLAGS_free;
  parameters.heuristic_period = FLAGS_heuristic_period;
  parameters.ignore_unknown = false;
  parameters.log_period = FLAGS_log_period;
  parameters.luby_restart = FLAGS_luby_restart;
  parameters.num_solutions = FLAGS_num_solutions;
  parameters.restart_log_size = FLAGS_restart_log_size;
  parameters.threads = FLAGS_workers;
  parameters.time_limit_in_ms = FLAGS_time_limit;
  parameters.use_log = FLAGS_fz_logging;
  parameters.verbose_impact = FLAGS_verbose_impact;
  parameters.worker_id = -1;
  parameters.search_type =
      FLAGS_use_impact ? FzSolverParameters::IBS : FzSolverParameters::DEFAULT;

  scoped_ptr<FzParallelSupportInterface> parallel_support(
      operations_research::MakeSequentialSupport(
          parameters.all_solutions, parameters.num_solutions));

  std::string problem_name(filename);
  problem_name.resize(problem_name.size() - 4);
  size_t found = problem_name.find_last_of("/\\");
  if (found != std::string::npos) {
    problem_name = problem_name.substr(found + 1);
  }
  FzModel model(problem_name);
  CHECK(ParseFlatzincFile(filename, &model));
  FzPresolve presolve;
  presolve.Init();
  CHECK(presolve.Run(&model));
  FzModelStatistics stats(model);
  stats.PrintStatistics();
  FzSolver solver(model);
  CHECK(solver.Extract());
  solver.Solve(parameters, parallel_support.get());
}

void FixAndParseParameters(int* argc, char*** argv) {
  FLAGS_log_prefix = false;
  char all_param[] = "--all";
  char free_param[] = "--free";
  char workers_param[] = "--workers";
  char solutions_param[] = "--num_solutions";
  char logging_param[] = "--fz_logging";
  for (int i = 1; i < *argc; ++i) {
    if (strcmp((*argv)[i], "-a") == 0) {
      (*argv)[i] = all_param;
    }
    if (strcmp((*argv)[i], "-f") == 0) {
      (*argv)[i] = free_param;
    }
    if (strcmp((*argv)[i], "-p") == 0) {
      (*argv)[i] = workers_param;
    }
    if (strcmp((*argv)[i], "-n") == 0) {
      (*argv)[i] = solutions_param;
    }
    if (strcmp((*argv)[i], "-l") == 0) {
      (*argv)[i] = logging_param;
    }
  }
  google::ParseCommandLineFlags( argc, argv, true);
  // Fix the number of solutions.
  if (FLAGS_num_solutions == 0) {  // not specified
    FLAGS_num_solutions = FLAGS_all ? kint32max : 1;
  }
}
}  // namespace operations_research

int main(int argc, char** argv) {
  operations_research::FixAndParseParameters(&argc, &argv);
  if (argc <= 1) {
    LOG(ERROR) << "Usage: " << argv[0] << " <file>";
    exit(EXIT_FAILURE);
  }
  operations_research::SequentialRun(argv[1]);
  return 0;
}
