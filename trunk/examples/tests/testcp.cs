// Copyright 2010-2012 Google
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

using System;
using Google.OrTools.ConstraintSolver;

public class CsTestCpOperator
{
  static void Check(bool test, String message)
  {
    if (!test)
    {
      Console.WriteLine("Error: " + message);
    }
  }

  static void CheckEquality(double v1, double v2, String message)
  {
    if (v1 != v2)
    {
      Console.WriteLine("Error: " + v1 + " != " + v2 + " " + message);
    }
  }

  static void TestConstructors()
  {
    Console.WriteLine("TestConstructors");
    Solver solver = new Solver("test");
    IntVar x = solver.MakeIntVar(0, 10, "x");
    Constraint c1 = x == 2;
    Console.WriteLine(c1.ToString());
    Constraint c2 = x >= 2;
    Console.WriteLine(c2.ToString());
    Constraint c3 = x > 2;
    Console.WriteLine(c3.ToString());
    Constraint c4 = x <= 2;
    Console.WriteLine(c4.ToString());
    Constraint c5 = x < 2;
    Console.WriteLine(c5.ToString());
    Constraint c6 = x != 2;
    Console.WriteLine(c6.ToString());
  }

  static void TestConstraintWithExpr()
  {
    Console.WriteLine("TestConstraintWithExpr");
    Solver solver = new Solver("test");
    IntVar x = solver.MakeIntVar(0, 10, "x");
    IntVar y = solver.MakeIntVar(0, 10, "y");
    Constraint c1 = x == 2;
    Constraint c2 = y == 3;
    IntExpr e2a = c1 + 1;
    Console.WriteLine(e2a.ToString());
    IntExpr e2b = 1 + c1;
    Console.WriteLine(e2b.ToString());
    IntExpr e2c = c1 - 1;
    Console.WriteLine(e2c.ToString());
    IntExpr e2d = 1 - c1;
    Console.WriteLine(e2d.ToString());
    IntExpr e2e = c1 * 2;
    Console.WriteLine(e2e.ToString());
    IntExpr e2f = 2 * c1;
    Console.WriteLine(e2f.ToString());
    IntExpr e3a = c1 + y;
    Console.WriteLine(e3a.ToString());
    IntExpr e3b = y + c1;
    Console.WriteLine(e3b.ToString());
    IntExpr e3c = c1 - y;
    Console.WriteLine(e3c.ToString());
    IntExpr e3d = y - c1;
    Console.WriteLine(e3d.ToString());
    IntExpr e3e = c1 * y;
    Console.WriteLine(e3e.ToString());
    IntExpr e3f = y * c1;
    Console.WriteLine(e3f.ToString());
    IntExpr e4 = -c1;
    Console.WriteLine(e4.ToString());
    IntExpr e5 = c1 / 4;
    Console.WriteLine(e5.ToString());
    IntExpr e6 = c1.Abs();
    Console.WriteLine(e6.ToString());
    IntExpr e7 = c1.Square();
    Console.WriteLine(e7.ToString());
    Constraint c8a = c1 == 1;
    Console.WriteLine(c8a.ToString());
    Constraint c8b = 1 == c1;
    Console.WriteLine(c8b.ToString());
    Constraint c8c = c1 != 1;
    Console.WriteLine(c8c.ToString());
    Constraint c8d = 1 != c1;
    Console.WriteLine(c8d.ToString());
    Constraint c8e = c1 >= 1;
    Console.WriteLine(c8e.ToString());
    Constraint c8f = 1 >= c1;
    Console.WriteLine(c8f.ToString());
    Constraint c8g = c1 > 1;
    Console.WriteLine(c8g.ToString());
    Constraint c8h = 1 > c1;
    Console.WriteLine(c8h.ToString());
    Constraint c8i = c1 <= 1;
    Console.WriteLine(c8i.ToString());
    Constraint c8j = 1 <= c1;
    Console.WriteLine(c8j.ToString());
    Constraint c8k = c1 < 1;
    Console.WriteLine(c8k.ToString());
    Constraint c8l = 1 < c1;
    Console.WriteLine(c8l.ToString());
    Constraint c9a = c1 == y;
    Console.WriteLine(c9a.ToString());
    Constraint c9b = y == c1;
    Console.WriteLine(c9b.ToString());
    Constraint c9c = c1 != y;
    Console.WriteLine(c9c.ToString());
    Constraint c9d = y != c1;
    Console.WriteLine(c9d.ToString());
    Constraint c9e = c1 >= y;
    Console.WriteLine(c9e.ToString());
    Constraint c9f = y >= c1;
    Console.WriteLine(c9f.ToString());
    Constraint c9g = c1 > y;
    Console.WriteLine(c9g.ToString());
    Constraint c9h = y > c1;
    Console.WriteLine(c9h.ToString());
    Constraint c9i = c1 <= y;
    Console.WriteLine(c9i.ToString());
    Constraint c9j = y <= c1;
    Console.WriteLine(c9j.ToString());
    Constraint c9k = c1 < y;
    Console.WriteLine(c9k.ToString());
    Constraint c9l = y < c1;
    Console.WriteLine(c9l.ToString());
    Constraint c10a = c1 == c2;
    Console.WriteLine(c10a.ToString());
    Constraint c10c = c1 != c2;
    Console.WriteLine(c10c.ToString());
    Constraint c10e = c1 >= c2;
    Console.WriteLine(c10e.ToString());
    Constraint c10g = c1 > c2;
    Console.WriteLine(c10g.ToString());
    Constraint c10i = c1 <= c2;
    Console.WriteLine(c10i.ToString());
    Constraint c10k = c1 < c2;
    Console.WriteLine(c10k.ToString());
    IntExpr e11a = c1 + (y == 2);
    Console.WriteLine(e11a.ToString());
    IntExpr e11b = (y == 2) + c1;
    Console.WriteLine(e11b.ToString());
    IntExpr e11c = c1 - (y == 2);
    Console.WriteLine(e11c.ToString());
    IntExpr e11d = (y == 2) - c1;
    Console.WriteLine(e11d.ToString());
    IntExpr e11e = c1 * (y == 2);
    Console.WriteLine(e11e.ToString());
    IntExpr e11f = (y == 2) * c1;
    Console.WriteLine(e11f.ToString());
    Constraint c12a = c1 == (y == 2);
    Console.WriteLine(c12a.ToString());
    Constraint c12b = (y == 2) == c1;
    Console.WriteLine(c12b.ToString());
    Constraint c12c = c1 != (y == 2);
    Console.WriteLine(c12c.ToString());
    Constraint c12d = (y == 2) != c1;
    Console.WriteLine(c12d.ToString());
    Constraint c12e = c1 >= (y == 2);
    Console.WriteLine(c12e.ToString());
    Constraint c12f = (y == 2) >= c1;
    Console.WriteLine(c12f.ToString());
    Constraint c12g = c1 > (y == 2);
    Console.WriteLine(c12g.ToString());
    Constraint c12h = (y == 2) > c1;
    Console.WriteLine(c12h.ToString());
    Constraint c12i = c1 <= (y == 2);
    Console.WriteLine(c12i.ToString());
    Constraint c12j = (y == 2) <= c1;
    Console.WriteLine(c12j.ToString());
    Constraint c12k = c1 < (y == 2);
    Console.WriteLine(c12k.ToString());
    Constraint c12l = (y == 2) < c1;
    Console.WriteLine(c12l.ToString());
  }

  static void TestWrappedConstraintWithExpr()
  {
    Console.WriteLine("TestWrappedConstraintWithExpr");
    Solver solver = new Solver("test");
    IntVar x = solver.MakeIntVar(0, 10, "x");
    IntVar y = solver.MakeIntVar(0, 10, "y");
    IntExpr e2a = (x == 1) + 1;
    Console.WriteLine(e2a.ToString());
    IntExpr e2b = 1 + (x == 1);
    Console.WriteLine(e2b.ToString());
    IntExpr e2c = (x == 1) - 1;
    Console.WriteLine(e2c.ToString());
    IntExpr e2d = 1 - (x == 1);
    Console.WriteLine(e2d.ToString());
    IntExpr e2e = (x == 1) * 2;
    Console.WriteLine(e2e.ToString());
    IntExpr e2f = 2 * (x == 1);
    Console.WriteLine(e2f.ToString());
    IntExpr e3a = (x == 1) + y;
    Console.WriteLine(e3a.ToString());
    IntExpr e3b = y + (x == 1);
    Console.WriteLine(e3b.ToString());
    IntExpr e3c = (x == 1) - y;
    Console.WriteLine(e3c.ToString());
    IntExpr e3d = y - (x == 1);
    Console.WriteLine(e3d.ToString());
    IntExpr e3e = (x == 1) * y;
    Console.WriteLine(e3e.ToString());
    IntExpr e3f = y * (x == 1);
    Console.WriteLine(e3f.ToString());
    IntExpr e4 = -(x == 1);
    Console.WriteLine(e4.ToString());
    IntExpr e5 = (x == 1) / 4;
    Console.WriteLine(e5.ToString());
    IntExpr e6 = (x == 1).Abs();
    Console.WriteLine(e6.ToString());
    IntExpr e7 = (x == 1).Square();
    Console.WriteLine(e7.ToString());
    Constraint c8a = (x == 1) == 1;
    Console.WriteLine(c8a.ToString());
    Constraint c8b = 1 == (x == 1);
    Console.WriteLine(c8b.ToString());
    Constraint c8c = (x == 1) != 1;
    Console.WriteLine(c8c.ToString());
    Constraint c8d = 1 != (x == 1);
    Console.WriteLine(c8d.ToString());
    Constraint c8e = (x == 1) >= 1;
    Console.WriteLine(c8e.ToString());
    Constraint c8f = 1 >= (x == 1);
    Console.WriteLine(c8f.ToString());
    Constraint c8g = (x == 1) > 1;
    Console.WriteLine(c8g.ToString());
    Constraint c8h = 1 > (x == 1);
    Console.WriteLine(c8h.ToString());
    Constraint c8i = (x == 1) <= 1;
    Console.WriteLine(c8i.ToString());
    Constraint c8j = 1 <= (x == 1);
    Console.WriteLine(c8j.ToString());
    Constraint c8k = (x == 1) < 1;
    Console.WriteLine(c8k.ToString());
    Constraint c8l = 1 < (x == 1);
    Console.WriteLine(c8l.ToString());
    Constraint c9a = (x == 1) == y;
    Console.WriteLine(c9a.ToString());
    Constraint c9b = y == (x == 1);
    Console.WriteLine(c9b.ToString());
    Constraint c9c = (x == 1) != y;
    Console.WriteLine(c9c.ToString());
    Constraint c9d = y != (x == 1);
    Console.WriteLine(c9d.ToString());
    Constraint c9e = (x == 1) >= y;
    Console.WriteLine(c9e.ToString());
    Constraint c9f = y >= (x == 1);
    Console.WriteLine(c9f.ToString());
    Constraint c9g = (x == 1) > y;
    Console.WriteLine(c9g.ToString());
    Constraint c9h = y > (x == 1);
    Console.WriteLine(c9h.ToString());
    Constraint c9i = (x == 1) <= y;
    Console.WriteLine(c9i.ToString());
    Constraint c9j = y <= (x == 1);
    Console.WriteLine(c9j.ToString());
    Constraint c9k = (x == 1) < y;
    Console.WriteLine(c9k.ToString());
    Constraint c9l = y < (x == 1);
    Console.WriteLine(c9l.ToString());
    Constraint c10a = (x == 1) == (y == 2);
    Console.WriteLine(c10a.ToString());
    Constraint c10c = (x == 1) != (y == 2);
    Console.WriteLine(c10c.ToString());
    Constraint c10e = (x == 1) >= (y == 2);
    Console.WriteLine(c10e.ToString());
    Constraint c10g = (x == 1) > (y == 2);
    Console.WriteLine(c10g.ToString());
    Constraint c10i = (x == 1) <= (y == 2);
    Console.WriteLine(c10i.ToString());
    Constraint c10k = (x == 1) < (y == 2);
    Console.WriteLine(c10k.ToString());
  }

  static void TestBaseEqualityWithExpr()
  {
    Console.WriteLine("TestBaseEqualityWithExpr");
    Solver solver = new Solver("test");
    IntVar x = solver.MakeIntVar(0, 10, "x");
    IntVar y = solver.MakeIntVar(0, 10, "y");
    IntExpr e2a = (x >= 1) + 1;
    Console.WriteLine(e2a.ToString());
    IntExpr e2b = 1 + (x >= 1);
    Console.WriteLine(e2b.ToString());
    IntExpr e2c = (x >= 1) - 1;
    Console.WriteLine(e2c.ToString());
    IntExpr e2d = 1 - (x >= 1);
    Console.WriteLine(e2d.ToString());
    IntExpr e2e = (x >= 1) * 2;
    Console.WriteLine(e2e.ToString());
    IntExpr e2f = 2 * (x >= 1);
    Console.WriteLine(e2f.ToString());
    IntExpr e3a = (x >= 1) + y;
    Console.WriteLine(e3a.ToString());
    IntExpr e3b = y + (x >= 1);
    Console.WriteLine(e3b.ToString());
    IntExpr e3c = (x >= 1) - y;
    Console.WriteLine(e3c.ToString());
    IntExpr e3d = y - (x >= 1);
    Console.WriteLine(e3d.ToString());
    IntExpr e3e = (x >= 1) * y;
    Console.WriteLine(e3e.ToString());
    IntExpr e3f = y * (x >= 1);
    Console.WriteLine(e3f.ToString());
    IntExpr e4 = -(x >= 1);
    Console.WriteLine(e4.ToString());
    IntExpr e5 = (x >= 1) / 4;
    Console.WriteLine(e5.ToString());
    IntExpr e6 = (x >= 1).Abs();
    Console.WriteLine(e6.ToString());
    IntExpr e7 = (x >= 1).Square();
    Console.WriteLine(e7.ToString());
    Constraint c8a = (x >= 1) >= 1;
    Console.WriteLine(c8a.ToString());
    Constraint c8b = 1 >= (x >= 1);
    Console.WriteLine(c8b.ToString());
    Constraint c8c = (x >= 1) != 1;
    Console.WriteLine(c8c.ToString());
    Constraint c8d = 1 != (x >= 1);
    Console.WriteLine(c8d.ToString());
    Constraint c8e = (x >= 1) >= 1;
    Console.WriteLine(c8e.ToString());
    Constraint c8f = 1 >= (x >= 1);
    Console.WriteLine(c8f.ToString());
    Constraint c8g = (x >= 1) > 1;
    Console.WriteLine(c8g.ToString());
    Constraint c8h = 1 > (x >= 1);
    Console.WriteLine(c8h.ToString());
    Constraint c8i = (x >= 1) <= 1;
    Console.WriteLine(c8i.ToString());
    Constraint c8j = 1 <= (x >= 1);
    Console.WriteLine(c8j.ToString());
    Constraint c8k = (x >= 1) < 1;
    Console.WriteLine(c8k.ToString());
    Constraint c8l = 1 < (x >= 1);
    Console.WriteLine(c8l.ToString());
    Constraint c9a = (x >= 1) >= y;
    Console.WriteLine(c9a.ToString());
    Constraint c9b = y >= (x >= 1);
    Console.WriteLine(c9b.ToString());
    Constraint c9c = (x >= 1) != y;
    Console.WriteLine(c9c.ToString());
    Constraint c9d = y != (x >= 1);
    Console.WriteLine(c9d.ToString());
    Constraint c9e = (x >= 1) >= y;
    Console.WriteLine(c9e.ToString());
    Constraint c9f = y >= (x >= 1);
    Console.WriteLine(c9f.ToString());
    Constraint c9g = (x >= 1) > y;
    Console.WriteLine(c9g.ToString());
    Constraint c9h = y > (x >= 1);
    Console.WriteLine(c9h.ToString());
    Constraint c9i = (x >= 1) <= y;
    Console.WriteLine(c9i.ToString());
    Constraint c9j = y <= (x >= 1);
    Console.WriteLine(c9j.ToString());
    Constraint c9k = (x >= 1) < y;
    Console.WriteLine(c9k.ToString());
    Constraint c9l = y < (x >= 1);
    Console.WriteLine(c9l.ToString());
    Constraint c10a = (x >= 1) >= (y >= 2);
    Console.WriteLine(c10a.ToString());
    Constraint c10c = (x >= 1) != (y >= 2);
    Console.WriteLine(c10c.ToString());
    Constraint c10e = (x >= 1) >= (y >= 2);
    Console.WriteLine(c10e.ToString());
    Constraint c10g = (x >= 1) > (y >= 2);
    Console.WriteLine(c10g.ToString());
    Constraint c10i = (x >= 1) <= (y >= 2);
    Console.WriteLine(c10i.ToString());
    Constraint c10k = (x >= 1) < (y >= 2);
    Console.WriteLine(c10k.ToString());
  }

  static void TestDowncast()
  {
    Solver solver = new Solver("TestDowncast");
    IntVar x = solver.MakeIntVar(0, 10, "x");
    IntExpr e = x + 1;
    IntVar y = e.Var();
    Console.WriteLine(y.ToString());
  }

  static void TestSequence()
  {
    Solver solver = new Solver("TestSequence");
    IntervalVar[] intervals =
        solver.MakeFixedDurationIntervalVarArray(10, 0, 10, 5, false, "task");
    DisjunctiveConstraint disjunctive = intervals.Disjunctive("Sequence");
    SequenceVar var = disjunctive.SequenceVar();
    Assignment ass = solver.MakeAssignment();
    ass.Add(var);
    ass.SetForwardSequence(var, new int[] { 1, 3, 5 });
    int[] seq = ass.ForwardSequence(var);
    Console.WriteLine(seq.Length);
  }

  // static void TestScheduling()
  // {
  //   Solver solver = new Solver("Scheduling");
  //   IntervalVar[] tasks = new IntervalVar[taskCount];
  //   List<IntVar> all_ends = new List<IntVar>();
  //   int i = 0;
  //   foreach(Task t in myTaskList)
  //   {
  //     tasks[i] = solver.MakeFixedInterval(0, (long)t.Duration, t.Name);
  //     if (t.Successors.Count <= 0)
  //       all_ends.Add(tasks[i].EndExpr().Var());
  //     //solver.Add(solver.MakeGreaterOrEqual(tasks[i].StartExpr(), 1)); // { 1 }
  //     i++;
  //   }

  //   IntVar objective_var = solver.MakeMax(all_ends.ToArray()).Var();
  //   OptimizeVar objective_monitor = solver.MakeMinimize(objective_var, 1);
  //   DecisionBuilder obj_phase = solver.MakePhase(objective_var, Solver.CHOOSE_FIRST_UNBOUND, Solver.ASSIGN_MIN_VALUE);

  //   const int kLogFrequency = 999999999;
  //   SearchMonitor search_log = solver.MakeSearchLog(kLogFrequency, objective_monitor);

  //   SolutionCollector collector = solver.MakeLastSolutionCollector();
  //   collector.AddObjective(objective_var);

  //   if (solver.Solve(obj_phase, objective_monitor))
  //   {
  //     Console.Out.WriteLine("Solution: ");
  //     foreach(IntervalVar t in tasks)
  //       Console.Out.WriteLine(t.ToString());
  //   }else
  //    {
  //      Console.Out.WriteLine("Can not find a solution");
  //    }
  // }

  static void Main()
  {
    TestConstructors();
    TestConstraintWithExpr();
    TestWrappedConstraintWithExpr();
    TestBaseEqualityWithExpr();
    TestDowncast();
    TestSequence();
  }
}
