// Copyright 2010-2021, Google Inc.
// All rights reserved.
//
// Redistribution and use in source and binary forms, with or without
// modification, are permitted provided that the following conditions are
// met:
//
//     * Redistributions of source code must retain the above copyright
// notice, this list of conditions and the following disclaimer.
//     * Redistributions in binary form must reproduce the above
// copyright notice, this list of conditions and the following disclaimer
// in the documentation and/or other materials provided with the
// distribution.
//     * Neither the name of Google Inc. nor the names of its
// contributors may be used to endorse or promote products derived from
// this software without specific prior written permission.
//
// THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
// "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
// LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR
// A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT
// OWNER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,
// SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT
// LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
// DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
// THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
// (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
// OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.

#include "base/container/arena.h"

#include <string>
#include <utility>
#include <vector>

#include "testing/gmock.h"
#include "testing/gunit.h"

using ::testing::ElementsAre;

namespace mozc {
namespace {

struct DestructorTracker {
  ~DestructorTracker() { seq.push_back(val); }

  int val;
  std::vector<int>& seq;
};

TEST(ArenaTest, BasicAllocation) {
  Arena<int> arena(2);

  int* p1 = arena.Alloc(1);
  int* p2 = arena.Alloc(2);
  int* p3 = arena.Alloc(3);

  EXPECT_EQ(*p1, 1);
  EXPECT_EQ(*p2, 2);
  EXPECT_EQ(*p3, 3);
}

TEST(ArenaTest, DestructorOrder) {
  std::vector<int> seq;
  {
    Arena<DestructorTracker> arena(2);

    static_cast<void>(arena.Alloc(1, seq));
    static_cast<void>(arena.Alloc(2, seq));
    static_cast<void>(arena.Alloc(3, seq));
  }

  EXPECT_THAT(seq, ElementsAre(3, 2, 1));
}

TEST(ArenaTest, Clear) {
  std::vector<int> seq;
  Arena<DestructorTracker> arena(10);

  static_cast<void>(arena.Alloc(1, seq));
  static_cast<void>(arena.Alloc(2, seq));
  arena.Clear();

  EXPECT_THAT(seq, ElementsAre(2, 1));

  static_cast<void>(arena.Alloc(3, seq));
  arena.Clear();

  EXPECT_THAT(seq, ElementsAre(2, 1, 3));
}

TEST(ArenaTest, Move) {
  Arena<int> arena1(10);
  int* p1 = arena1.Alloc(42);

  Arena<int> arena2(std::move(arena1));
  int* p2 = arena2.Alloc(43);

  Arena<int> arena3(42);
  arena3 = std::move(arena2);

  EXPECT_EQ(*p1, 42);
  EXPECT_EQ(*p2, 43);
}

TEST(ObjectPoolTest, Reuse) {
  ObjectPool<std::string> pool(10);

  std::string* s1 = pool.Alloc("hello");
  void* addr1 = s1;
  pool.Release(s1);
  std::string* s2 = pool.Alloc("world");
  void* addr2 = s2;

  EXPECT_EQ(addr1, addr2);
}

}  // namespace
}  // namespace mozc
