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

#ifndef MOZC_DICTIONARY_POS_ID_MAP_H_
#define MOZC_DICTIONARY_POS_ID_MAP_H_

#include <cstddef>
#include <cstdint>
#include <vector>

#include "absl/strings/string_view.h"

namespace mozc::dictionary {

// Manages POS ID to POS string mappings.
// Typically initialized with the content of 'pos_id_map.data' packaged in
// mozc.data.
//
// Binary format:
// Null-separated UTF-8 strings. The N-th null-separated token corresponds to
// POS ID N.
class PosIdMap {
 public:
  // Initializes the map from serialized binary data.
  // If the data is invalid or empty, it initializes an empty map.
  //
  // NOTE: The buffer `pos_id_map_data` must outlive the `PosIdMap` instance
  // because the internal table holds `absl::string_view` pointing directly
  // into the buffer.
  explicit PosIdMap(absl::string_view pos_id_map_data);

  PosIdMap(const PosIdMap&) = delete;
  PosIdMap& operator=(const PosIdMap&) = delete;

  ~PosIdMap() = default;

  // Returns the POS string corresponding to the given POS ID.
  // Returns an empty string view if the POS ID is invalid or not found.
  // Example: 12 -> "副詞,一般,*,*,*,*,*"
  // NOTE: The returned string view points to the buffer provided to the
  // constructor. It must not outlive the buffer.
  absl::string_view GetPosString(uint16_t pos_id) const;

  // Returns the total number of POS IDs.
  size_t GetPosIdCount() const;

 private:
  // Maps POS ID to POS string.
  // The index is the POS ID.
  std::vector<absl::string_view> pos_id_to_string_table_;
};

}  // namespace mozc::dictionary

#endif  // MOZC_DICTIONARY_POS_ID_MAP_H_
