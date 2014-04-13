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

namespace Google.OrTools.Util
{
  using System;
  using System.Collections.Generic;

  public static class NestedArrayHelper
  {
    public static T[] GetFlatArray<T>(T[,] arr) {
      int flatLength = arr.GetLength(0) * arr.GetLength(1);
      int idx = 0;
      T[] flat = new T[flatLength];

      for (int i = 0; i < arr.GetLength(0); i++)
        for (int j = 0; j < arr.GetLength(1); j++)
          flat[idx++] = arr[i,j];

      return flat;
    }
  }
}  // namespace Google.OrTools.Util
