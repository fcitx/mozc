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

#include "data_manager/testing/mock_data_manager.h"

#include <cstddef>

#include "absl/log/check.h"
#include "base/embedded_file.h"
#include "data_manager/data_manager.h"

namespace mozc {
namespace testing {
namespace {

// EmbeddedFile kMockMozcDataSet is defined in this header file.
#include "data_manager/testing/mock_mozc_data.inc"

#ifndef MOZC_DATASET_MAGIC_NUMBER_LENGTH
#error "MOZC_DATASET_MAGIC_NUMBER_LENGTH is not defined by build system"
#endif  // MOZC_DATASET_MAGIC_NUMBER_LENGTH

constexpr size_t kMagicNumberLength = MOZC_DATASET_MAGIC_NUMBER_LENGTH;

}  // namespace

MockDataManager::MockDataManager() {
  CHECK_OK(DataManager::InitFromArray(LoadEmbeddedFile(kMockMozcDataSet),
                                      kMagicNumberLength))
      << "Embedded mock_mozc_data.h is broken";
}

}  // namespace testing
}  // namespace mozc
