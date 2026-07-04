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

#ifndef MOZC_CONVERTER_CACHING_CONNECTOR_H_
#define MOZC_CONVERTER_CACHING_CONNECTOR_H_

#include <array>
#include <cstdint>
#include <limits>

#include "absl/algorithm/container.h"
#include "absl/log/check.h"
#include "converter/connector.h"

namespace mozc {

// A wrapper for Connector to minimize calls of Connector::GetTransitionCost()
// in Viterbi algorithm. This way the performance of Viterbi algorithm improves
// significantly in terms of time consumption because
// Connector::GetTransitionCost() is slow due to its compression format. This
// class implements a cache strategy designed for the access pattern in Viterbi
// algorithm.
//
// In Viterbi algorithm, the connection matrix is looked up in a nested loop as
// follows:
//
// for each right node `r`:
//   for each left node `l`:
//     ...
//     transition cost = connector.GetTransitionCost(l.rid, r.lid)
//     ...
//
// Therefore, in the inner loop, `r.lid` is fixed. So we can simply use an array
// to cache the transition cost for (l.rid, r.lid) in `cache[l.rid]`, and cache
// is reset before the inner loop. Moreover, right nodes are likely to be
// ordered by `r.lid` although it's not guaranteed in general. This fact is plus
// for this caching strategy as we only need to reset the cache if `r.lid` is
// different from the previous value.
//
// NOTE: This class is designed only for Viterbi algorithm and won't work for
// other purposes.
class CachingConnector final {
 public:
  explicit CachingConnector(const Connector& connector)
      : connector_{connector} {}

  CachingConnector(const CachingConnector&) = delete;
  CachingConnector& operator=(const CachingConnector&) = delete;

  void ResetCacheIfNecessary(uint16_t rnode_lid) {
    if (cache_lid_ != rnode_lid) {
      absl::c_fill(cache_, -1);
      cache_lid_ = rnode_lid;
    }
  }

  int GetTransitionCost(uint16_t lnode_rid, uint16_t rnode_lid) {
    DCHECK_EQ(cache_lid_, rnode_lid);
    // Values for rid >= kCacheSize cannot be cached. However, frequent PoSs
    // have smaller IDs, so caching only for rid in [0, kCacheSize) works well.
    if (lnode_rid >= kCacheSize) {
      return connector_.GetTransitionCost(lnode_rid, rnode_lid);
    }
    if (cache_[lnode_rid] != -1) {
      return cache_[lnode_rid];
    }
    cache_[lnode_rid] = connector_.GetTransitionCost(lnode_rid, rnode_lid);
    return cache_[lnode_rid];
  }

 private:
  constexpr static int kCacheSize = 2048;

  const Connector& connector_;
  std::array<int, kCacheSize> cache_;
  uint16_t cache_lid_ = std::numeric_limits<uint16_t>::max();
};

}  // namespace mozc

#endif  // MOZC_CONVERTER_CACHING_CONNECTOR_H_
